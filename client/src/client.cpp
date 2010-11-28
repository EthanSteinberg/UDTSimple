#include "client.h"
#include "../network/network.h"
#include <boost/filesystem.hpp>
#include <cstdio>
#include <iostream>
#include <gtk/gtk.h>
#include <fstream>

#include <udt.h>
#include <arpa/inet.h>

using namespace std;

boost::mutex mOrderDataMutex;
boost::mutex mConnectMutex;
boost::condition_variable mOrderDataUpdate;
boost::condition_variable mConnectUpdate;

struct windowAndFileList
{
   GtkListStore *orderlist;
   GtkWidget *fileviewer;
   queue<orderData> *mOrderData;
   UDTSOCKET *client;
   int *haveFiles;
};

struct connectData
{
   GtkWidget *dialog;
   GtkEntryBuffer *entryBoxBuffer;
   UDTSOCKET *client;
   UDTSOCKET *clientDAT;
   int *connect;
   string *lastIp;
   GtkWidget *toplabel;
   int *haveFiles;
   GtkListStore *filelist;
};

struct topData
{
   GtkWidget *window;
   connectData ConData;
};

struct windowData
{
   UDTSOCKET *client;
   UDTSOCKET *clientDAT;
   int *connect;
};

static gboolean delete_event(GtkWidget*, GdkEvent*,gpointer);
static void destroy( GtkWidget *, gpointer);
static gboolean fileaddpushed(GtkWidget *,gpointer );
static gboolean topbuttonpushed(GtkWidget *,gpointer);
static gboolean connectpushed(GtkWidget *,gpointer );
static void showError(GtkWidget *root);

void Client::go(int argc, char **argv)
{
   gtk_init (&argc, &argv);

   lastIp = "not connected";
   string topLabelInfo = "Connected to: ";
   connected = 0;
   haveFiles = 0;

   createWindow();
   
   makeOrderList();
   makeFileList();
   
   addfilebutton = gtk_button_new_with_label("Download the file");
   topbutton = gtk_button_new_with_label("Connect to server");
   toplabel = gtk_label_new((topLabelInfo  + lastIp).c_str());
   topprogress = gtk_progress_bar_new();

   gtk_progress_bar_set_text(GTK_PROGRESS_BAR(topprogress),"No file");

   makeMajorBoxes();
   showStuff();  

   startNetworkThread();

   windowAndFileList buttonstor = {orderlist, fileviewer, &mOrderData, &client, &haveFiles};
   g_signal_connect (addfilebutton, "clicked", G_CALLBACK (fileaddpushed), &buttonstor);

   connectData conDat = {NULL,NULL,&client, &clientDAT, &connected, &lastIp, toplabel, &haveFiles, filelist};
   topData topDat = {window,conDat};
   g_signal_connect (topbutton, "clicked", G_CALLBACK (topbuttonpushed), &topDat);
   
   windowData winDat = {&client,&clientDAT, &connected};
   g_signal_connect (window, "delete-event", G_CALLBACK (delete_event), &winDat);
   
   gdk_threads_enter();
   gtk_main();
   gdk_threads_leave();
}

void Client::makeFileList()
{
   filelist = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
   
   GtkTreeViewColumn *column;
   GtkTreeViewColumn *column2;
   GtkCellRenderer *renderer;

   renderer = gtk_cell_renderer_text_new();
   column = gtk_tree_view_column_new_with_attributes("File Name",renderer,"text",0,NULL);
   column2 = gtk_tree_view_column_new_with_attributes("Size",renderer,"text",1,NULL);

   fileviewer = gtk_tree_view_new_with_model(GTK_TREE_MODEL(filelist));
   gtk_tree_view_append_column(GTK_TREE_VIEW(fileviewer), column);
   gtk_tree_view_append_column(GTK_TREE_VIEW(fileviewer), column2);

   filescroll = gtk_scrolled_window_new(NULL,NULL);
   gtk_container_add (GTK_CONTAINER (filescroll), fileviewer);
   GtkAdjustment *hadjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(filescroll));
   gtk_tree_view_set_vadjustment(GTK_TREE_VIEW(fileviewer),hadjust);
}

void Client::makeOrderList()
{
   orderlist = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
   
   GtkTreeViewColumn *column;
   GtkTreeViewColumn *column2;
   GtkCellRenderer *renderer;

   renderer = gtk_cell_renderer_text_new();
   column = gtk_tree_view_column_new_with_attributes("File wanted",renderer,"text",0,NULL);
   column2 = gtk_tree_view_column_new_with_attributes("Status",renderer,"text",1,NULL);

   orderviewer = gtk_tree_view_new_with_model(GTK_TREE_MODEL(orderlist));
   gtk_tree_view_append_column(GTK_TREE_VIEW(orderviewer), column);
   gtk_tree_view_append_column(GTK_TREE_VIEW(orderviewer), column2);

   orderscroll = gtk_scrolled_window_new(NULL,NULL);
   gtk_container_add (GTK_CONTAINER (orderscroll), orderviewer);
   GtkAdjustment *hadjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(orderscroll));
   gtk_tree_view_set_vadjustment(GTK_TREE_VIEW(orderviewer),hadjust);
}


void Client::makeMajorBoxes()
{
   leftbox = gtk_vbox_new(FALSE,0);
   rightbox = gtk_vbox_new(FALSE,0);
   mainpane = gtk_hpaned_new();
   mainbox = gtk_vbox_new(FALSE,0);
   topbox = gtk_hbox_new(FALSE,0);

   leftframe = gtk_frame_new("Files");
   gtk_frame_set_shadow_type(GTK_FRAME(leftframe),GTK_SHADOW_IN);
   gtk_frame_set_label_align(GTK_FRAME(leftframe),.5,1);
   gtk_container_add (GTK_CONTAINER (leftframe), leftbox);
   
   rightframe = gtk_frame_new("Orders");
   gtk_frame_set_shadow_type(GTK_FRAME(rightframe),GTK_SHADOW_IN);
   gtk_frame_set_label_align(GTK_FRAME(rightframe),.5,1);
   gtk_container_add (GTK_CONTAINER (rightframe), rightbox);

   gtk_paned_add1(GTK_PANED(mainpane), leftframe);
   gtk_paned_add2(GTK_PANED(mainpane), rightframe);

   gtk_box_pack_start(GTK_BOX (leftbox), filescroll,TRUE,TRUE,3);
   gtk_box_pack_start(GTK_BOX (leftbox), addfilebutton,FALSE,FALSE,3);

   gtk_box_pack_start(GTK_BOX (rightbox), orderscroll,TRUE,TRUE,3);
   
   gtk_box_pack_start(GTK_BOX (mainbox), topbox,FALSE,FALSE,3);
   gtk_box_pack_start(GTK_BOX (mainbox), mainpane,TRUE,TRUE,3);
   
   gtk_box_pack_start(GTK_BOX (topbox), toplabel,FALSE,FALSE,3);
   gtk_box_pack_start(GTK_BOX (topbox), topbutton,FALSE,FALSE,3);
   gtk_box_pack_start(GTK_BOX (topbox), topprogress,FALSE,FALSE,3);
   
   gtk_container_add (GTK_CONTAINER (window), mainbox);
}  

void Client::showStuff()
{
   //Show lists
   gtk_widget_show (fileviewer);
   gtk_widget_show (filescroll);
    
   gtk_widget_show (orderviewer);
   gtk_widget_show (orderscroll);
   
   //Show major boxes/frames
   gtk_widget_show (mainpane);
   gtk_widget_show (mainbox);
   gtk_widget_show (topbox);
   gtk_widget_show (leftbox);
   gtk_widget_show (leftframe);
   gtk_widget_show (rightbox);
   gtk_widget_show (rightframe);

   gtk_widget_show (addfilebutton);
   gtk_widget_show (toplabel);
   gtk_widget_show (topbutton);
   gtk_widget_show (topprogress);

   /* and the window */
   gtk_widget_show (window);
}  

void Client::createWindow()
{
   /* create a new window */
   window = gtk_window_new (GTK_WINDOW_TOPLEVEL); 
   g_signal_connect (window, "destroy", G_CALLBACK (destroy), NULL);
   gtk_container_set_border_width (GTK_CONTAINER (window), 10);

}
   
static gboolean delete_event(GtkWidget*, GdkEvent*,gpointer data)
{
   windowData *stuff = reinterpret_cast<windowData *>(data);
   
   if(*(stuff->connect))
   {
      {
         boost::lock_guard<boost::mutex> lock(mConnectMutex);

         *(stuff->connect) = 0;
      }

      t_disconectPacket disconectPacket;

      if (UDT::ERROR == UDT::sendmsg(*(stuff->client),(char *) &disconectPacket, sizeof(disconectPacket),-1,true))
      {
         cout << "sendquit: " << UDT::getlasterror().getErrorMessage();
         exit(0);
      }
      
      UDT::close(*(stuff->client));
      UDT::close(*(stuff->clientDAT));
   }

   return FALSE;
}

static void destroy( GtkWidget *, gpointer)
{
    gtk_main_quit ();
}

static gboolean topbuttonpushed(GtkWidget *,gpointer windowptr)
{
   topData *stuff = (topData *) windowptr;
   
   GtkWidget *dialog;
   GtkWidget *actionArea;
   GtkWidget *vboxArea;
   GtkWidget *text;
   GtkWidget *button;
   GtkWidget *entryBox;
   GtkEntryBuffer *entryBoxBuffer;

   entryBoxBuffer = gtk_entry_buffer_new(stuff->ConData.lastIp->c_str(),-1);
   entryBox = gtk_entry_new_with_buffer(entryBoxBuffer);

   text = gtk_label_new("Please insert an ip then press accept:  ");
   button = gtk_button_new_with_label("Accept");

   dialog = gtk_dialog_new_with_buttons("Please instert an ip",GTK_WINDOW(stuff->window),GTK_DIALOG_DESTROY_WITH_PARENT,GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,NULL);
   actionArea = gtk_dialog_get_action_area(GTK_DIALOG(dialog));
   vboxArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

   gtk_box_pack_start(GTK_BOX (actionArea), button,FALSE,FALSE,3);
   gtk_box_pack_start(GTK_BOX (vboxArea), text,FALSE,FALSE,3);
   gtk_box_pack_start(GTK_BOX (vboxArea), entryBox,FALSE,FALSE,3);

   gtk_widget_show(button);
   gtk_widget_show(text);
   gtk_widget_show(entryBox);
   
   stuff->ConData.dialog = dialog;
   stuff->ConData.entryBoxBuffer = entryBoxBuffer;

   g_signal_connect (button, "clicked", G_CALLBACK (connectpushed), &(stuff->ConData));
   
   gtk_dialog_run(GTK_DIALOG(dialog));
   gtk_widget_destroy(dialog);
   
   return TRUE;
}


static gboolean fileaddpushed(GtkWidget *,gpointer windowptr)
{
   windowAndFileList *stuff = (windowAndFileList *) windowptr;

   GtkTreeSelection *selection;
   if ((selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(stuff->fileviewer))) && *(stuff->haveFiles))
   {
      GtkTreeIter iter;
      GtkTreeModel *model;
      gtk_tree_selection_get_selected(selection,&model, &iter);

      char *buffer;
      int size; 
      gtk_tree_model_get(model,&iter, 0, &buffer,2,&size,-1);

      boost::filesystem::path lol(buffer);
      std::string filename = lol.filename();
      
      char printfBuffer[100];
      int i = 0;
      while (boost::filesystem::exists(filename))
      {
         snprintf(printfBuffer,sizeof(printfBuffer),"%s(%d)",lol.filename().c_str(),i++);
         filename = printfBuffer;
      }
      
      //create the file 
      {
         ofstream lol(filename);
      }

      cout<<filename<<endl;
      cout<<filename.c_str()<<endl;

      printf("Added %s with size %d to the queue\n",filename.c_str(),size);

      GtkTreeIter testiter;
      gtk_list_store_append(stuff->orderlist,&testiter);
      gtk_list_store_set(stuff->orderlist,&testiter,0, filename.c_str() , 1, "In queue",-1);

      orderData temp;
      temp.name = filename;
      temp.size = size;
      temp.iter = testiter;

      t_wantPacket wantData;
      strncpy(wantData.name,buffer,sizeof(wantData.name));
      wantData.size = size;

      {
         boost::lock_guard<boost::mutex> lock(mOrderDataMutex);
         
         stuff->mOrderData->push(temp);
      }

      mOrderDataUpdate.notify_one();
      
      if (UDT::ERROR == UDT::sendmsg(*(stuff->client),(char *) &wantData, sizeof(wantData),-1,true))
      {
         cout << "sendIwantIt: " << UDT::getlasterror().getErrorMessage();
         exit(0);
      }

   }
   
   return TRUE;
}

static gboolean connectpushed(GtkWidget *,gpointer data)
{
   connectData *stuff(reinterpret_cast<connectData *>(data));

   if(*(stuff->connect))
   {
      {
         boost::lock_guard<boost::mutex> lock(mConnectMutex);

         *(stuff->connect) = 0;
      }

      t_disconectPacket disconectPacket;

      if (UDT::ERROR == UDT::sendmsg(*(stuff->client),(char *) &disconectPacket, sizeof(disconectPacket),-1,true))
      {
         cout << "sendquit: " << UDT::getlasterror().getErrorMessage();
         exit(0);
      }
      
      *(stuff->lastIp) = "not connected";
      
      string labelstring = "Connected to: ";
      gtk_label_set_text(GTK_LABEL(stuff->toplabel),(labelstring + "not connected").c_str());

      *(stuff->haveFiles) = 0;

      gtk_list_store_clear(stuff->filelist);

      UDT::close(*(stuff->client));
      UDT::close(*(stuff->clientDAT));
   }

   const char* address = gtk_entry_buffer_get_text(stuff->entryBoxBuffer);   

   sockaddr_in serv_addr;
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_port = htons(9000);
   inet_pton(AF_INET, address, &serv_addr.sin_addr);

   memset(&(serv_addr.sin_zero), '\0', 8);

   sockaddr_in serv_addrDAT;
   serv_addrDAT.sin_family = AF_INET;
   serv_addrDAT.sin_port = htons(9001);
   inet_pton(AF_INET, address, &serv_addrDAT.sin_addr);

   memset(&(serv_addrDAT.sin_zero), '\0', 8);


   // connect to the server, implict bind
   if (UDT::ERROR == UDT::connect(*(stuff->client), (sockaddr*)&serv_addr, sizeof(serv_addr)))
   {
      cout << "connect: " <<address<< UDT::getlasterror().getErrorMessage();
      showError(stuff->dialog);
      return FALSE;
   }

   if (UDT::ERROR == UDT::connect(*(stuff->clientDAT), (sockaddr*)&serv_addrDAT, sizeof(serv_addrDAT)))
   {
      cout << "DATconnect: " <<address<< UDT::getlasterror().getErrorMessage();
      showError(stuff->dialog);
      return FALSE;
   }

   string labelstring = "Connected to: ";
   gtk_label_set_text(GTK_LABEL(stuff->toplabel),(labelstring + address).c_str());

   *(stuff->lastIp) = address;
   
   {
      boost::lock_guard<boost::mutex> lock(mConnectMutex);

      *(stuff->connect) = 1;
   }
   
   mConnectUpdate.notify_all();
   
   gtk_dialog_response(GTK_DIALOG(stuff->dialog),1);
   return TRUE;
}

static void showError(GtkWidget *root)
{
   GtkWidget *dialog;
   dialog = gtk_message_dialog_new (GTK_WINDOW(root),
                                 GTK_DIALOG_DESTROY_WITH_PARENT,
                                 GTK_MESSAGE_ERROR,
                                 GTK_BUTTONS_CLOSE,
                                 "Error connecting");
   gtk_dialog_run (GTK_DIALOG (dialog));
   gtk_widget_destroy (dialog);
}
