#include "client.h"
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>

#include <udt.h>
#include <arpa/inet.h>
#include <fstream>
#include <iostream>
#include <cstring>
#include <string>

#include "../network/network.h"
#include <cstdlib>

using namespace std;

struct updateData
{
   GtkWidget *progressbar;
   int size;
   string name;
   char buffer[100];
};


gboolean updateProgress(gpointer data)
{
   updateData *stuff = reinterpret_cast<updateData *>(data);

   int cursize = boost::filesystem::file_size(stuff->name);

   gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(stuff->progressbar),(double) cursize/stuff->size);

   snprintf(stuff->buffer,sizeof(stuff->buffer),"%s : %.2f mb /%.2f mb",stuff->name.c_str(),(float) cursize/1000000,(float) stuff->size/1000000);
   gtk_progress_bar_set_text(GTK_PROGRESS_BAR(stuff->progressbar),stuff->buffer);

   if (cursize == stuff->size)
      return FALSE;

   return TRUE;
}


void networkControlThread(UDTSOCKET socket,GtkListStore *filelist,int *connected, int *haveFiles)
{
   char buffer[200];
   
   while (1)
   {
      {
         boost::unique_lock<boost::mutex> lock(mConnectMutex);

         if (!*connected)
            mConnectUpdate.wait(lock);
      }

      if (UDT::ERROR == UDT::recvmsg(socket,buffer, sizeof(buffer)))
      {
         cout << "recvNextMessage: " << UDT::getlasterror().getErrorMessage();
      }
      
      switch (reinterpret_cast<t_Packet *>(buffer)->type)
      {
         case 2:
         {
            char buff[100];
            t_wantPacket wantPacket(*reinterpret_cast<t_wantPacket *>(buffer));

            cout<<wantPacket.name<<' '<<wantPacket.size<<endl;
            int size = wantPacket.size;
            snprintf(buff,sizeof(buff),"%d",size);

            gdk_threads_enter();

            GtkTreeIter testiter;
            gtk_list_store_append(filelist,&testiter);
            gtk_list_store_set(filelist,&testiter,0,wantPacket.name,1,buff ,2,size,-1);

            gdk_threads_leave();

            *haveFiles = 1;
         }
      }
   }

   UDT::close(socket);
}

void networkDataThread(UDTSOCKET socket, std::queue<orderData> *mOrderData, GtkListStore *orderlist, int *connected, GtkWidget *progressbar)
{
   orderData CurrentOrder;

   while(1)
   {
      {
         boost::unique_lock<boost::mutex> lock(mConnectMutex);

         if (!*connected)
            mConnectUpdate.wait(lock);
      }

      {
         boost::unique_lock<boost::mutex> lock(mOrderDataMutex);

         if (mOrderData->empty())
            mOrderDataUpdate.wait(lock);

         CurrentOrder = mOrderData->front();
         mOrderData->pop();
      }

      updateData tempUpdate = {progressbar,CurrentOrder.size,CurrentOrder.name};      


      gdk_threads_enter();

      gtk_list_store_set(orderlist,&(CurrentOrder.iter),1, "Downloading", -1);
      g_timeout_add(100,(GSourceFunc) updateProgress,&tempUpdate);

      gdk_threads_leave();
    

      fstream ofs(CurrentOrder.name);

      if (UDT::ERROR == UDT::recvfile(socket, ofs, 0, CurrentOrder.size))
      {
         cout << "recvfile: " << UDT::getlasterror().getErrorMessage();
         exit(0); 
      }
      
      gdk_threads_enter();

      gtk_list_store_set(orderlist,&(CurrentOrder.iter),1, "Done", -1);

      gdk_threads_leave();
   }

   UDT::close(socket);
}

void Client::startNetworkThread()
{
   boost::thread newthread(boost::bind(&Client::startNetwork,this)); 
}

void Client::startNetwork()
{
   client = UDT::socket(AF_INET, SOCK_DGRAM, 0);
   clientDAT = UDT::socket(AF_INET, SOCK_STREAM, 0);

   boost::thread controlThread(boost::bind(networkControlThread,client,filelist,&connected,&haveFiles));

   boost::thread dataThread(boost::bind(networkDataThread,clientDAT,&mOrderData,orderlist,&connected,topprogress));

   cout<<endl;

}
