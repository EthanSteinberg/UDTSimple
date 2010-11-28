#include "server.h"

#include "../network/network.h"

#include <udt.h>
#include <arpa/inet.h>

#include <iostream>
#include <cstdio>
#include <gtk/gtk.h>
#include <fstream>

using namespace std;

boost::mutex mClientDataMutex;
boost::mutex mFileDataMutex;

struct windowAndFileList
{
   GtkListStore *filelist;
   GtkWidget *window;
   GtkWidget *fileviewer;
   set<fileData> &mFileData;
   set<clientData> *mClientData;
};

static gboolean delete_event(GtkWidget*, GdkEvent*,gpointer);
static void destroy( GtkWidget *, gpointer);
static gboolean fileaddpushed(GtkWidget *,gpointer *);

void Server::go(int argc, char **argv)
{
   startNetworkThread();
   gtk_init (&argc, &argv);

   createWindow();
   
   addfilebutton = gtk_button_new_with_label("Add a file");

   makeFileList();
   windowAndFileList buttonstor = {filelist, window, fileviewer,mFileData, &mClientData};
   g_signal_connect (addfilebutton, "clicked", G_CALLBACK (fileaddpushed), &buttonstor);
   
   makeOrderList();

   makeMajorBoxes();
   showStuff();  

   gdk_threads_enter();
   gtk_main();
   gdk_threads_leave();
}

void Server::makeFileList()
{
   filelist = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
   
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

void Server::makeOrderList()
{
   orderlist = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
   
   GtkTreeViewColumn *column;
   GtkTreeViewColumn *column2;
   GtkCellRenderer *renderer;

   renderer = gtk_cell_renderer_text_new();
   column = gtk_tree_view_column_new_with_attributes("IP",renderer,"text",0,NULL);
   column2 = gtk_tree_view_column_new_with_attributes("Downloading what",renderer,"text",1,NULL);

   orderviewer = gtk_tree_view_new_with_model(GTK_TREE_MODEL(orderlist));
   gtk_tree_view_append_column(GTK_TREE_VIEW(orderviewer), column);
   gtk_tree_view_append_column(GTK_TREE_VIEW(orderviewer), column2);

   orderscroll = gtk_scrolled_window_new(NULL,NULL);
   gtk_container_add (GTK_CONTAINER (orderscroll), orderviewer);
   GtkAdjustment *hadjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(orderscroll));
   gtk_tree_view_set_vadjustment(GTK_TREE_VIEW(orderviewer),hadjust);
}


void Server::makeMajorBoxes()
{
   leftbox = gtk_vbox_new(FALSE,0);
   rightbox = gtk_vbox_new(FALSE,0);
   mainpane = gtk_hpaned_new();

   leftframe = gtk_frame_new("Files");
   gtk_frame_set_shadow_type(GTK_FRAME(leftframe),GTK_SHADOW_IN);
   gtk_frame_set_label_align(GTK_FRAME(leftframe),.5,1);
   gtk_container_add (GTK_CONTAINER (leftframe), leftbox);
   
   rightframe = gtk_frame_new("Clients");
   gtk_frame_set_shadow_type(GTK_FRAME(rightframe),GTK_SHADOW_IN);
   gtk_frame_set_label_align(GTK_FRAME(rightframe),.5,1);
   gtk_container_add (GTK_CONTAINER (rightframe), rightbox);

   gtk_paned_add1(GTK_PANED(mainpane), leftframe);
   gtk_paned_add2(GTK_PANED(mainpane), rightframe);

   gtk_box_pack_start((GtkBox*) leftbox, filescroll,TRUE,TRUE,3);
   gtk_box_pack_start((GtkBox*) leftbox, addfilebutton,FALSE,FALSE,3);

   gtk_box_pack_start((GtkBox*) rightbox, orderscroll,TRUE,TRUE,3);
   
   gtk_container_add (GTK_CONTAINER (window), mainpane);
}  

void Server::showStuff()
{
   //Show lists
   gtk_widget_show (fileviewer);
   gtk_widget_show (filescroll);
    
   gtk_widget_show (orderviewer);
   gtk_widget_show (orderscroll);
   
   //Show major boxes/frames
   gtk_widget_show (mainpane);
   gtk_widget_show (leftbox);
   gtk_widget_show (leftframe);
   gtk_widget_show (rightbox);
   gtk_widget_show (rightframe);

   gtk_widget_show (addfilebutton);

   /* and the window */
   gtk_widget_show (window);
}  

void Server::createWindow()
{
   /* create a new window */
   window = gtk_window_new (GTK_WINDOW_TOPLEVEL); 
   g_signal_connect (window, "delete-event", G_CALLBACK (delete_event), NULL);
   g_signal_connect (window, "destroy", G_CALLBACK (destroy), NULL);
   gtk_container_set_border_width (GTK_CONTAINER (window), 10);

}
   
static gboolean delete_event(GtkWidget*, GdkEvent*,gpointer)
{
    return FALSE;
}

static void destroy( GtkWidget *, gpointer)
{
    gtk_main_quit ();
}

static gboolean fileaddpushed(GtkWidget *,gpointer *windowptr)
{
   windowAndFileList *stuff = (windowAndFileList *) windowptr;;

   GtkWidget *dialog;
   dialog = gtk_file_chooser_dialog_new ("Open File",
                  GTK_WINDOW(stuff->window),
                  GTK_FILE_CHOOSER_ACTION_OPEN,
                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                  GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                  NULL);
   if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
   {
      char *filename;
      char buffer[30];
      

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
            
      fstream ifs(filename);

      ifs.seekg(0,ios::end);
      int size = ifs.tellg();
      ifs.seekg(0,ios::beg);


      fileData temp;
      temp.name = filename;
      temp.size = size;
     
      int exists;
      {
         boost::lock_guard<boost::mutex> lock(mFileDataMutex);

         exists = stuff->mFileData.count(temp);
      }

      if (!exists)
      {
         snprintf(buffer,sizeof(buffer),"%d",size);

         GtkTreeIter testiter;
         gtk_list_store_append(stuff->filelist,&testiter);
         gtk_list_store_set(stuff->filelist,&testiter,0, filename, 1, buffer,-1);
         {
            boost::lock_guard<boost::mutex> lock(mFileDataMutex);

            stuff->mFileData.insert(temp);
         }

         t_wantPacket wantPacket;
         strncpy(wantPacket.name,filename,sizeof(wantPacket.name));
         wantPacket.size = size;
      
         {
            boost::lock_guard<boost::mutex> lock(mClientDataMutex);

            for (auto iter = stuff->mClientData->begin();iter != stuff->mClientData->end(); iter++)
            {
               const clientData &client = *iter;
               printf("I am sending the file %s to %s.\n",filename,client.ip.c_str());
            
               if (UDT::ERROR == UDT::sendmsg(*(client.socket),(char *) &wantPacket ,sizeof(wantPacket), -1,true))
               {
                  cout << "send:" << UDT::getlasterror().getErrorMessage() << endl;
               }
            }
         }
      }
   }
   gtk_widget_destroy (dialog);

   return TRUE;
}
