#ifndef SERVER_H_INCLUDED
#define SERVER_H_INCLUDED

#include <gtk/gtk.h>
#include <udt.h>

#include <string>
#include <boost/thread.hpp>
#include <set>
#include <queue>

struct fileData
{
   std::string name;
   int size;

   bool operator<(const fileData &other) const
   {
      return name<other.name;
   }
};

struct orderData
{
   std::string name;
   int size;
   int orderNum;

   GtkTreeIter iter;
};

class Client
{
public:
   Client() {}

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
   GtkWidget *mainbox;
   GtkWidget *topbox;
   
   GtkWidget *toplabel;
   GtkWidget *topprogress;
   
   //buttons
   GtkWidget *addfilebutton;
   GtkWidget *topbutton;

   //list viewers
   GtkWidget *fileviewer;
   GtkWidget *filescroll;
   GtkListStore *filelist;

   GtkWidget *orderviewer;
   GtkWidget *orderscroll;
   GtkListStore *orderlist;

   std::queue<orderData> mOrderData;

   UDTSOCKET client;
   UDTSOCKET clientDAT;

   std::string lastIp;
   int connected;

   int haveFiles;
};
   
extern boost::mutex mConnectMutex;
extern boost::mutex mOrderDataMutex;
extern boost::condition_variable mOrderDataUpdate;
extern boost::condition_variable mConnectUpdate;

#endif
