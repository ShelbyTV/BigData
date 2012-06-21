#include <fstream>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <limits>
#include <algorithm>

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>
#include <assert.h>

#include "lib/mongo-c-driver/src/mongo.h"

#include "lib/compact-language-detector/src/encodings/compact_lang_det/compact_lang_det.h"
#include "lib/compact-language-detector/src/encodings/compact_lang_det/ext_lang_enc.h"
#include "lib/compact-language-detector/src/encodings/compact_lang_det/unittest_data.h"
#include "lib/compact-language-detector/src/encodings/proto/encodings.pb.h"

using namespace std;

static mongo conn;
static map <string, bool> alreadySeen;

bool isEnglish(const string &comment)
{
   bool is_plain_text = true;
   bool do_allow_extended_languages = true;
   bool do_pick_summary_language = false;
   bool do_remove_weak_matches = false;
   bool is_reliable;
   const char* tld_hint = NULL;
   int encoding_hint = UNKNOWN_ENCODING;
   Language language_hint = UNKNOWN_LANGUAGE;

   double normalized_score3[3];
   Language language3[3];
   int percent3[3];
   int text_bytes;

   const char* src = comment.c_str();
   Language lang;
   lang = CompactLangDet::DetectLanguage(0,
                                         src, strlen(src),
                                         is_plain_text,
                                         do_allow_extended_languages,
                                         do_pick_summary_language,
                                         do_remove_weak_matches,
                                         tld_hint,
                                         encoding_hint,
                                         language_hint,
                                         language3,
                                         percent3,
                                         normalized_score3,
                                         &text_bytes,
                                         &is_reliable);
   
   //printf("LANG=%s, isReliable=%s\n", LanguageName(lang), is_reliable ? "YES" : "NO");
   return lang == ENGLISH;
}

void printHelpText()
{
   cout << "mongoCommentsGetter usage:" << endl;
   cout << "   -h --help        Print this help message" << endl;
}

bool connectToMongoAndAuthenticate()
{
   mongo_init(&conn);

   mongo_replset_init(&conn, "shelbySet");
   mongo_replset_add_seed(&conn, "10.181.131.101", 27018);
   mongo_replset_add_seed(&conn, "10.181.135.130", 27018);
   mongo_replset_add_seed(&conn, "10.181.131.114", 27018);

   int status = mongo_replset_connect( &conn );

   if (MONGO_OK != status) {
      switch (conn.err) {
         case MONGO_CONN_SUCCESS:    break;
         case MONGO_CONN_NO_SOCKET:  printf("no socket\n"); return false;
         case MONGO_CONN_FAIL:       printf("connection failed\n"); return false;
         case MONGO_CONN_NOT_MASTER: printf("not master\n"); return false;
         default:                    printf("received unknown status\n"); return false;
      }
   }

   return true;
}

void getMongoBroadcastComments()
{
   bson_oid_t oldest;
   bson_oid_from_string(&oldest, "4fcbbbccbb2dac0de20001c5");

   bson_oid_t newest;
   bson_oid_from_string(&newest, "4fd4fbccbb2dac0de20001c5");

   bson query;
   bson_init(&query);
   bson_append_start_object(&query, "$query");

   bson_append_start_object(&query, "_id");
   bson_append_oid(&query, "$gte", &oldest);
   bson_append_oid(&query, "$lte", &newest);
   bson_append_finish_object(&query);

   bson_append_finish_object(&query);
   bson_finish(&query);

   bson fields;
   bson_init(&fields);
//   bson_append_int(&fields, "A", 1);
//   bson_append_int(&fields, "c", 1);
   bson_append_int(&fields, "y", 1);
   bson_append_int(&fields, "z", 1);
   bson_finish(&fields);

   mongo_cursor cursor;
   mongo_cursor_init(&cursor, &conn, "nos-production.broadcasts");
   mongo_cursor_set_query(&cursor, &query);
   mongo_cursor_set_fields(&cursor, &fields);
//   mongo_cursor_set_limit(&cursor, 100);

   while (mongo_cursor_next(&cursor) == MONGO_OK) {
      bson_iterator iterator_id;
      bson_iterator iterator_comment;
//      bson_iterator iterator_origin;
//      bson_iterator iterator_nickname;

      if(
//         bson_find(&iterator_origin, mongo_cursor_bson(&cursor), "A") &&
//         bson_find(&iterator_nickname, mongo_cursor_bson(&cursor), "c") &&
         bson_find(&iterator_id, mongo_cursor_bson(&cursor), "y") &&
         bson_find(&iterator_comment, mongo_cursor_bson(&cursor), "z"))
      {
         string id = bson_iterator_string(&iterator_id);
         string comment = bson_iterator_string(&iterator_comment);
         transform(comment.begin(), comment.end(), comment.begin(), ::tolower);

//         string origin = bson_iterator_string(&iterator_origin);
//         string nickname = bson_iterator_string(&iterator_nickname);

         replace(comment.begin(), comment.end(), '\n', ' ');
         replace(comment.begin(), comment.end(), '\r', ' ');

         if (alreadySeen.find(id) == alreadySeen.end()) {
           alreadySeen.insert(pair<string, bool>(id, true));
           if (isEnglish(comment)) {
              cout << comment << endl;
           }
         }
      }      
   }
 
   mongo_cursor_destroy(&cursor);
   bson_destroy(&fields);
   bson_destroy(&query);
}

void parseUserOptions(int argc, char **argv)
{
   int c;
     
   while (1) {
      static struct option long_options[] =
      {
         {"help",        no_argument,       0, 'h'},
         {0, 0, 0, 0}
      };
      
      int option_index = 0;
      c = getopt_long(argc, argv, "h", long_options, &option_index);
   
      /* Detect the end of the options. */
      if (c == -1) {
         break;
      }
   
      switch (c)
      {
         case 'h': 
         case '?':
         default:
            printHelpText();
            exit(1);
      }
   }
}

int main(int argc, char **argv)
{
   int status = 0;

   parseUserOptions(argc, argv);

   if (!connectToMongoAndAuthenticate()) {
      status = 1;
      goto mongoCleanup;
   } 

//   cout << "Connected to mongo." << endl;

   getMongoBroadcastComments();

mongoCleanup:
   mongo_destroy(&conn);
   return status;
}
