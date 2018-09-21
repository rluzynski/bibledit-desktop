/*
 ** Copyright (©) 2016-2018 Matt Postiff.
 **  
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 3 of the License, or
 ** (at your option) any later version.
 **  
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **  
 ** You should have received a copy of the GNU General Public License
 ** along with this program; if not, write to the Free Software
 ** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **  
 */

#include "libraries.h"
#include <glib.h>
#include "windowtabbed.h"
#include "help.h"
#include "floatingwindow.h"
#include "keyterms.h"
#include "tiny_utilities.h"
#include "utilities.h"
#include <gdk/gdkkeysyms.h>
#include "combobox.h"
#include "settings.h"
#include "projectutils.h"
#include "categorize.h"
#include "mapping.h"
#include "bible.h"
#include "books.h"
#include "xmlutils.h"
#include "gwrappers.h"
#include <glib/gi18n.h>
#include "concordance.h"

WindowTabbed::WindowTabbed(ustring _title, GtkWidget * parent_layout, GtkAccelGroup *accelerator_group, bool startup):
  FloatingWindow(parent_layout, widTabbed, _title, startup)
  // widTabbed above may not be specific enough for what windowdata wants to do (storing window positions, etc.)
  // May pass that in; also have to pass in title info since it will vary; this is a generic container class
{
  title = _title;
  newReference = NULL;
  // Build gui.
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox);
  gtk_container_add(GTK_CONTAINER(vbox_client), vbox);

  // The contents of this vbox--the notebook with its tabs
  notebook = gtk_notebook_new();
  gtk_widget_show(notebook);
  gtk_notebook_set_tab_pos((GtkNotebook *)notebook, GTK_POS_TOP);
  gtk_container_add(GTK_CONTAINER(vbox), notebook);

  // Produce the signal to be given on a new reference.
  signalVerseChange = gtk_button_new();
  gtk_box_pack_start(GTK_BOX(vbox), signalVerseChange, FALSE, FALSE, 0);
}

WindowTabbed::~WindowTabbed()
{
    vector<SingleTab *> tabs;
    for (auto t: tabs) {
      delete t;   
    }
    newReference = NULL;
    active_url = "";
    myreference.assign(0, 0, "");
    title = "";
    gtk_widget_destroy(vbox);
    // The above should also destroy the GtkWidget *notebook
    gtk_widget_destroy(signalVerseChange);
}

SingleTab::SingleTab(const ustring &_title, HtmlWriter2 &html, GtkWidget *notebook, WindowTabbed *_parent)
{
    title = _title;
    parent = _parent;
    // WebKit2: WebKitWebView is scrollable by itself, so you don't need to embed it in a GtkScrolledWindow.

    webview = webkit_web_view_new();
    gtk_widget_show (webview);

    tab_label = gtk_label_new_with_mnemonic (title.c_str());
    gtk_notebook_append_page((GtkNotebook *)notebook, webview, tab_label);
   
    parent->connect_focus_signals (webview); // this routine is inherited from FloatingWindow
    
    webkit_web_view_load_html (WEBKIT_WEB_VIEW (webview), html.html.c_str(), NULL);
    // TO DO: Scroll to the position that possibly was stored while this url was last active.
    //GtkAdjustment * adjustment = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolledwindow));
    //gtk_adjustment_set_value (adjustment, scrolling_position[active_url]);
    
    g_signal_connect((gpointer) webview, "decide-policy", G_CALLBACK(on_decide_policy_cb), gpointer(this));
}

SingleTab::~SingleTab()
{
    title = "";
    parent = NULL;
    gtk_widget_destroy(webview);
    // I think the above should take care of all its kids (?) 
    // and I believe that notebook will be taken care of in the parent when it
    // destroys its stuff.
}

void WindowTabbed::newTab(const ustring &tabTitle, HtmlWriter2 &tabHtml)
{
  // Create a new tab (notebook page)
    SingleTab *newTab = new SingleTab(tabTitle, tabHtml, notebook, this);
    tabs.push_back(newTab);
}

void WindowTabbed::updateTab(const ustring &tabTitle, HtmlWriter2 &tabHtml)
{
    SingleTab *existingTab = NULL;
    // Find tab by its title, then update the html that it contains
    for (auto it : tabs) {
        if (it->title == tabTitle) {
            existingTab = it;
        }
    }
    assert(existingTab != NULL);
    existingTab->updateHtml(tabHtml);
}

void SingleTab::updateHtml(HtmlWriter2 &html)
{
   // cerr << "HTML=" << html.html.c_str() << endl;
   webkit_web_view_load_html (WEBKIT_WEB_VIEW (webview), html.html.c_str(), NULL);
}

gboolean
SingleTab::on_decide_policy_cb (WebKitWebView           *web_view,
				WebKitPolicyDecision    *decision,
				WebKitPolicyDecisionType decision_type,
				gpointer                 user_data)
{
  ((SingleTab *) user_data)->decide_policy_cb (web_view, decision, decision_type);
  // For above, see webview_simple.cpp
  return true;
}

#if 0
void SingleTab::decide_policy_cb (WebKitWebView           *web_view,
				  WebKitPolicyDecision    *decision,
				  WebKitPolicyDecisionType decision_type)
{
// Callback for clicking a link.
//#if 0
  // I don't know how to do this with web_view...since it is not contained in a scrolledwindow anymore...
  // Store scrolling position for the now active url.
  GtkAdjustment * adjustment = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolledwindow_terms));
  scrolling_position[active_url] = gtk_adjustment_get_value (adjustment);
  
  DEBUG("remember old scroll position="+std::to_string(scrolling_position[active_url])+" for old active_url="+active_url)
//#endif
    switch (decision_type) {
    case WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION:
      {
        WebKitNavigationPolicyDecision *navigation_decision = WEBKIT_NAVIGATION_POLICY_DECISION (decision);
	WebKitNavigationAction         *navigation_action =
	  webkit_navigation_policy_decision_get_navigation_action(navigation_decision);
	WebKitNavigationType           navigation_type = 
	  webkit_navigation_action_get_navigation_type(navigation_action);
	
	switch (navigation_type) {
	case WEBKIT_NAVIGATION_TYPE_LINK_CLICKED: { // The navigation was triggered by clicking a link.
	  // Don't follow pseudo-links clicked on this page.
	  webkit_policy_decision_ignore (decision);
	  
	  WebKitURIRequest *request = webkit_navigation_action_get_request (navigation_action);
	  const gchar *uri = webkit_uri_request_get_uri (request);

  	  // Load new page depending on the pseudo-link clicked.
	  webview_process_navigation (uri);
	}
	  break;
	case WEBKIT_NAVIGATION_TYPE_FORM_SUBMITTED: // The navigation was triggered by submitting a form.
	  // fall through
	case WEBKIT_NAVIGATION_TYPE_BACK_FORWARD: // The navigation was triggered by navigating forward or backward.
	  // fall through
	case WEBKIT_NAVIGATION_TYPE_RELOAD: // The navigation was triggered by reloading.
	  // fall through
	case WEBKIT_NAVIGATION_TYPE_FORM_RESUBMITTED: // The navigation was triggered by resubmitting a form.
	  // fall through
	case WEBKIT_NAVIGATION_TYPE_OTHER: // The navigation was triggered by some other action.
	  webkit_policy_decision_use (decision);
	  break;
	}
      }
      break;
	
    case WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION:
      {
        WebKitNavigationPolicyDecision *navigation_decision = WEBKIT_NAVIGATION_POLICY_DECISION (decision);
        /* Make a policy decision here. */
	webkit_policy_decision_use (decision);
      }
        break;
    case WEBKIT_POLICY_DECISION_TYPE_RESPONSE:
      {
        WebKitResponsePolicyDecision *response = WEBKIT_RESPONSE_POLICY_DECISION (decision);
        /* Make a policy decision here. */
	webkit_policy_decision_use (decision);
      }
        break;
    default:
      /* Making no decision results in webkit_policy_decision_use(). */
      return;
    }
  return;
  // return value handled by the above callback func; maybe shouldn't be.
}
#endif

// This is reproduced from windowcheckkeyterms.cpp, and should be 
// factored out into the reference class.
Reference SingleTab::get_reference (const ustring& text)
// Generates a reference out of the text.
{
  Reference ref;
  ustring book, chapter, verse = ref.verse_get(); // not sure why verse_get is called?
  decode_reference(text, book, chapter, verse); // probably also factor this out
  ref.book_set( books_localname_to_id (book));
  ref.chapter_set(convert_to_int (chapter));
  ref.verse_set(verse);
  return ref;
}

extern Concordance *concordance;

void SingleTab::webview_process_navigation (const ustring &url)
{
  // Store scrolling position for the now active url. (Is this duplicated from above?)
  //GtkAdjustment * adjustment = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolledwindow_terms));
  //scrolling_position[active_url] = gtk_adjustment_get_value (adjustment);

  //DEBUG("remember old scroll position="+std::to_string(scrolling_position[active_url])+" for old active_url="+active_url)
  //DEBUG("active_url="+active_url+" new url="+ustring(url))

  // New url.
  parent->active_url = url;
  ustring myurl = url;

  // In the case that this link is a "goto [verse]" link, we can process it
  // right here and now.
  if (myurl.find ("goto ") == 0) {
    // Signal the editors to go to a reference.
    myurl.erase (0, 5); // get rid of keyword "goto" and space
    cout << "Visiting verse >> " << myurl << endl;
    parent->myreference.assign (get_reference (myurl));
    parent->newReference = &parent->myreference;
    gtk_button_clicked(GTK_BUTTON(parent->signalVerseChange)); // parent->signalVerseChange is in WindowTabbed, not in this single tab
  }
  // Something more complicated like "concordance [some concordance word]" will mean that we have to 
  // send for help from the producer of the data that is presently in this tab.
  // The link "keyword" tells us what to do.
  else if (myurl.find ("concordance ") == 0) {
    // Create a new concordance tab and fill it with the word list
    myurl.erase (0, 12); // get rid of keyword "concordance" and space
    // The remainder of myurl is the word we need to look up in the concordance
    HtmlWriter2 html("");
    concordance->writeSingleWordListHtml(myurl, html);
    html.finish();
    parent->newTab(myurl, html);
  }
  // See similar method in windowcheckkeyterms for more possible stuff to do
}
