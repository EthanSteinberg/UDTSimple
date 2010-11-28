#include "server.h"
#include <boost/thread.hpp>

#include <udt.h>
#include <arpa/inet.h>
#include <fstream>
#include <iostream>
#include <cstring>
#include <string>

#include "../network/network.h"

using namespace std;

void threadTheSocket(UDTSOCKET sock, UDTSOCKET sockDAT, GtkListStore *orderlist, std::string ip,set<clientData> *mClientData, set<fileData> *mFileData)
{
   clientData temp;
   temp.ip = ip;
   temp.socket = &sock;

   {
      boost::lock_guard<boost::mutex> lock(mClientDataMutex);

      mClientData->insert(temp);
   }

   std::set<fileData> tempFileData;
   {
      boost::lock_guard<boost::mutex> lock(mFileDataMutex);

      tempFileData = *mFileData;
   }
   
   t_wantPacket filePacket;
   for (auto iter = tempFileData.begin(); iter != tempFileData.end(); ++iter)
   {
      const fileData &tempFile = *iter;
      
      strncpy(filePacket.name,tempFile.name.c_str(),sizeof(filePacket.name));
      filePacket.size = tempFile.size;
      
      if (UDT::ERROR == UDT::sendmsg(sock,(char *) &filePacket ,sizeof(filePacket), -1,true))
      {
         cout << "sendfile:" << UDT::getlasterror().getErrorMessage() << endl;
      }
   }

   gdk_threads_enter();
   
   GtkTreeIter testiter;
   gtk_list_store_append(orderlist,&testiter);
   gtk_list_store_set(orderlist,&testiter,0,ip.c_str() ,-1);

   gdk_threads_leave();

   char buffer[200];
   
   while (1)
   {
      if (UDT::ERROR == UDT::recvmsg(sock,buffer, sizeof(buffer)))
      {
         cout << "recv: " << UDT::getlasterror().getErrorMessage();
         exit(0);
      }
      
      switch (reinterpret_cast<t_Packet *>(buffer)->type)
      {
         case 2:
         {
            t_wantPacket wantPacket(*reinterpret_cast<t_wantPacket *>(buffer));
            
            fstream ifs(wantPacket.name);

            gdk_threads_enter();
   
            gtk_list_store_set(orderlist,&testiter,1,wantPacket.name ,-1);

            gdk_threads_leave();
            
            if (UDT::ERROR == UDT::sendfile(sockDAT,ifs ,0 , wantPacket.size))
            {
               cout << "sendfile:" << UDT::getlasterror().getErrorMessage() << endl;
            }
            
            gdk_threads_enter();
   
            gtk_list_store_set(orderlist,&testiter,1,"" ,-1);

            gdk_threads_leave();
         }
            break;

         case 3:
         {
            goto quit;   
         }
      }
   }
   
   quit:
   
   {
      boost::lock_guard<boost::mutex> lock(mClientDataMutex);

      mClientData->erase(temp);
   }
   
   gdk_threads_enter();
   gtk_list_store_remove(orderlist,&testiter);
   gdk_threads_leave();

   UDT::close(sock);
   UDT::close(sockDAT);
}

void Server::startNetworkThread()
{
   boost::thread newthread(boost::bind(&Server::startNetwork,this)); 
}

void Server::startNetwork()
{
   UDTSOCKET serv = UDT::socket(AF_INET, SOCK_DGRAM, 0);

   sockaddr_in my_addr;
   my_addr.sin_family = AF_INET;
   my_addr.sin_port = htons(9000);
   my_addr.sin_addr.s_addr = INADDR_ANY;
   memset(&(my_addr.sin_zero), '\0', 8);

   if (UDT::ERROR == UDT::bind(serv, (sockaddr*)&my_addr, sizeof(my_addr)))
   {
      cout << "serv bind: " << UDT::getlasterror().getErrorMessage();
   }

   UDT::listen(serv, 1000);

   
   UDTSOCKET servDAT = UDT::socket(AF_INET, SOCK_STREAM, 0);

   sockaddr_in my_addrDAT;
   my_addrDAT.sin_family = AF_INET;
   my_addrDAT.sin_port = htons(9001);
   my_addrDAT.sin_addr.s_addr = INADDR_ANY;
   memset(&(my_addrDAT.sin_zero), '\0', 8);

   if (UDT::ERROR == UDT::bind(servDAT, (sockaddr*)&my_addrDAT, sizeof(my_addrDAT)))
   {
      cout << "servDAT bind: " << UDT::getlasterror().getErrorMessage();
   }

   UDT::listen(servDAT, 1000);
   
   
   int namelen;
   sockaddr_in their_addr;

   cout<< UDT::getlasterror().getErrorMessage()<<endl;
   
   char buffer[100];
   while (1)
   {
      UDTSOCKET recver = UDT::accept(serv, (sockaddr*)&their_addr, &namelen);
      UDTSOCKET recverDAT = UDT::accept(servDAT, (sockaddr*)&their_addr, &namelen);
     
      snprintf(buffer,sizeof(buffer),"%s:%d",inet_ntoa(their_addr.sin_addr),ntohs(their_addr.sin_port));
      boost::thread threadForSock(boost::bind(threadTheSocket,recver,recverDAT,orderlist,buffer,&mClientData,&mFileData));
   }

   UDT::close(serv);

   cout<<endl;
}
