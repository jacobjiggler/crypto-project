#include <iostream>
#include <string>
#include <cstdlib>
#include "cryptopp/cryptlib.h"
#include "cryptopp/filters.h"
#include "cryptopp/aes.h"
#include "cryptopp/gcm.h"
#include "cryptopp/modes.h"
#include "cryptopp/osrng.h"
#include <pthread.h>

using namespace std;
using namespace CryptoPP;


enum rettypes {SUCCESS = 0, FAILURE = -1};

void generate_iv(byte iv[]){
  AutoSeededRandomPool rnd;
  memset( iv, 0, sizeof(iv) );
  rnd.GenerateBlock(iv, 12);
}

int encrypt(char* keyin, char* ptxt, const byte iv[], char*& ctxt, char*& oadata){

AutoSeededRandomPool rnd;
  
byte key[32]; memset( key, 0, sizeof(key) );
byte rndadata[16]; memset(rndadata, 0 , sizeof(rndadata));

 cout << "IVIN: " <<  iv << endl;
 

string thekey = "0123456789012345678901234567890";
strcpy(key, keyin);

 cout <<"key: "<< key << endl;
 

rnd.GenerateBlock(rndadata, 16); 
string adata( (char *) rndadata, 16);
string pdata(ptxt);

cout <<"ptxt: " << pdata << endl;

const int TAG_SIZE = 16;

// Encrypted, with Tag
string cipher, encoded;

// Recovered (decrypted)
string radata, rpdata;

/*********************************\
\*********************************/

try
  {
    GCM< AES >::Encryption e;
    e.SetKeyWithIV( key, sizeof(key), iv, sizeof(iv) );

    AuthenticatedEncryptionFilter ef( e,
				      new StringSink( cipher ), false,
				      TAG_SIZE /* MAC_AT_END */
				      ); // AuthenticatedEncryptionFilter

    // AuthenticatedEncryptionFilter::ChannelPut
    //  defines two channels: DEFAULT_CHANNEL and AAD_CHANNEL
    //   DEFAULT_CHANNEL is encrypted and authenticated
    //   AAD_CHANNEL is authenticated
    ef.ChannelPut( AAD_CHANNEL, adata.data(), adata.size() );
    ef.ChannelMessageEnd(AAD_CHANNEL);

    // Authenticated data *must* be pushed before
    //  Confidential/Authenticated data. Otherwise
    //  we must catch the BadState exception
    ef.ChannelPut( DEFAULT_CHANNEL, pdata.data(), pdata.size() );
    ef.ChannelMessageEnd(DEFAULT_CHANNEL);
  }
 catch( CryptoPP::Exception& e )
   {
     cerr << "Caught Exception..." << endl;
     cerr << e.what() << endl;
     cerr << endl;
     return FAILURE;
   }

/*********************************\
\*********************************/

//
// The pair { adata, cipher } is sent to
//  the other party or persisted to storage
//

// Attack the first and last byte of the
//  encrypted data and tag
//if( cipher.size() > 1 )
//{
//   cipher[ 0 ] |= 0x0F;
//   cipher[ cipher.size()-1 ] |= 0x0F;
//}

/*********************************\
\*********************************/
cout << cipher << endl;
ctxt =  strdup(cipher.c_str()); //copy the cipher text and adata
cout << ctxt << endl;
oadata = strdup(adata.c_str());
//cout << oadata << endl;
return SUCCESS;
}

char* decrypt(byte iv[], char* ikey, char* iadata, char* icipher){
byte key[32]; memset( key, 0, sizeof(key) );
cout << "IV: " << endl;
strcpy(key, ikey);

const int TAG_SIZE = 16;

string adata(iadata);
string pdata( 18, (char)0x00 );

// Encrypted, with Tag
string cipher(icipher), encoded;

// Recovered (decrypted)
string radata, rpdata;

int flag = 0;
char* out;

try
  {
    GCM< AES >::Decryption d;
    d.SetKeyWithIV( key, sizeof(key), iv, sizeof(iv) );

    // Break the cipher text out into it's
    //  components: Encrypted and MAC
    string enc = cipher.substr( 0, cipher.length()-TAG_SIZE );
    string mac = cipher.substr( cipher.length()-TAG_SIZE );

    // Sanity checks
    // assert( cipher.size() == enc.size() + mac.size() );
    //assert( enc.size() == pdata.size() ); LIVING ON THE EDGE
    //assert( TAG_SIZE == mac.size() );

    // Not recovered - sent via clear channel
    radata = adata;

    // Object *will* throw an exception
    //  during decryption\verification _if_
    //  verification fails.
    AuthenticatedDecryptionFilter df( d, NULL,
				      AuthenticatedDecryptionFilter::MAC_AT_BEGIN | AuthenticatedDecryptionFilter::THROW_EXCEPTION, TAG_SIZE );

    // The order of the following calls are important
    df.ChannelPut( DEFAULT_CHANNEL, mac.data(), mac.size() );
    df.ChannelPut( AAD_CHANNEL, adata.data(), adata.size() );
    df.ChannelPut( DEFAULT_CHANNEL, enc.data(), enc.size() );

    // If the object throws, it will most likely occur
    //   during ChannelMessageEnd()
    df.ChannelMessageEnd( AAD_CHANNEL );
    df.ChannelMessageEnd( DEFAULT_CHANNEL );

    // If the object does not throw, here's the only
    //  opportunity to check the data's integrity
    bool b = false;
    b = df.GetLastResult();
    assert( true == b );

    // Remove data from channel
    string retrieved;
    size_t n = (size_t)-1;

    // Plain text recovered from enc.data()
    df.SetRetrievalChannel( DEFAULT_CHANNEL );
    n = (size_t)df.MaxRetrievable();
    retrieved.resize( n );

    if( n > 0 ) { df.Get( (byte*)retrieved.data(), n ); }
    rpdata = retrieved;
    //assert( rpdata == pdata ); I AM EDGE

    // All is well - work with data
    cout << "Decrypted and Verified data. Ready for use." << endl;
    cout << endl;

    cout << "adata length: " << adata.size() << endl;
    cout << "pdata length: " << pdata.size() << endl;
    cout << endl;

    cout << "recovered adata length: " << radata.size() << endl;
    cout << "recovered pdata length: " << rpdata.size() << endl;
    cout << endl;
  }
 catch( CryptoPP::Exception& e )
   {
     cerr << "Caught Exception..." << endl;
     cerr << e.what() << endl;
     cerr << endl;
     flag = 1;
   }
 if (flag == 0)
   out = strdup(rpdata.c_str());
 else
   out = "Error!";
 return out;
}

int encrypt_send(char* key, char* &data){
  char* oadata;
  char* octxt;
  char* outtext;
  byte iv[12];
  generate_iv(iv);
  if (encrypt(key, data, iv, octxt, oadata) == FAILURE){
    return FAILURE;
  }
  string oadatas(oadata);
  string octxts(octxt);
  string ivs((char*) iv);
  string output;
  output = ivs + "-|-" + oadatas + "-|-" + octxts + "-|-";
  cout <<"ThisOut: " << output << endl;
  data = strdup(output.c_str());
  return SUCCESS;
}

int decrypt_receive(char* key, char* &data){
  string all(data);
  string ivd = all.substr(0, all.find("-|-"));
  cout << "IVD: " << ivd << endl;
  string iadata = all.substr(ivd.size() + 3, 16);
  cout << "IADATA: " << iadata << endl;
  string ictxt = all.substr(ivd.size() + iadata.size() + 6, all.size() - (ivd.size() + iadata.size() + 6) - 3);
  byte iv[12];
  strcpy(iv, (char*)ivd.c_str());
  string out(decrypt(iv, key, iadata.c_str(), ictxt.c_str()));
  if (out == "ERROR"){
    return FAILURE;
  }
  data = strdup(out.c_str());
  return SUCCESS;
}
