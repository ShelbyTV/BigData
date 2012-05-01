#include "lib/mongo-c-driver/src/mongo.h"

int main()
{
   mongo conn;
   mongo_init(&conn);

   int status = mongo_connect(&conn, "127.0.0.1", 27018);

   if( status != MONGO_OK ) {
      switch ( conn.err ) {
         case MONGO_CONN_SUCCESS:    printf( "connection succeeded\n" ); break;
         case MONGO_CONN_NO_SOCKET:  printf( "no socket\n" ); return 1;
         case MONGO_CONN_FAIL:       printf( "connection failed\n" ); return 1;
         case MONGO_CONN_NOT_MASTER: printf( "not master\n" ); return 1;
         default:                    printf( "received unknown status\n"); return 1;
      }
   } else {
       printf( "connected: MONGO_OK\n\n" );
   }

   mongo_destroy(&conn);
   return 0;
}
