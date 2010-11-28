#ifndef SERVER_H_INCLUDED
#define SERVER_H_INCLUDED

#include <gtk/gtk.h>
#include <udt.h>

#include <string>
#include <boost/thread.hpp>
#include <set>

struct fileData
{
   std::string name;
   int size;

   bool operator<(const fileData &other) const
   {
      return name<other.name;
   }
};

struct clientData
{
   std::string ip;

   UDTSOCKET *socket;

   bool operator<(const clientData &other) const
   {
      return ip<other.ip;
   }
};

class Server
{
public:
   Server() {}

   void go(int argc, char **argv);

private:
   void showStuff();
   void makeMajorBoxes();
   void makeOrderList();
   void makeFileList();
   void createWindow();

   void startNetworkThread();
   void startNetwork();

   //Main frames
   GtkWidget *leftframe;
   GtkWidget *rightframe;

   //Major boxes
   GtkWidget *mainpane;
   GtkWidget *window;
   GtkWidget *leftbox;
   GtkWidget *rightbox;
   
   //buttons
   GtkWidget *addfilebutton;

   //list viewers
   GtkWidget *fileviewer;
   GtkWidget *filescroll;
   GtkListStore *filelist;

   GtkWidget *orderviewer;
   GtkWidget *orderscroll;
   GtkListStore *orderlist;

   std::set<fileData> mFileData;
   std::set<clientData> mClientData;
};
   
extern boost::mutex mClientDataMutex;
extern boost::mutex mFileDataMutex;

#endif
