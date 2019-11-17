/*
 * HAS_Update.cpp
 *
 * (C) 2019 James Dixon
 *
 */

#include "HAS_Update.h"
#include "HAS_option.h"
#include "config.h"
#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPClient.h>

String UpdateURL;
bool UpdateNow = false;
bool Updating = false;

void TestServerUpdate()
{
  if( !Updating )
  { // if we're not updating, then we must have restarted, hopefully the new version!
    // first check the version is what we are actually running...
    if( strcmp(options.NewVersion(), options.Version()) != 0  )
    {
      options.Version( options.NewVersion() );
      // let's just check...
      String newver = options.Version();
      int upos = newver.lastIndexOf("_");
      newver = newver.substring(upos+1);
      if( newver != P_VERSION )
      {
        Serial.print("**WARNING: Image version ");
        Serial.print(options.Version());
        Serial.print(" (");
        Serial.print(options.NewVersion());
        Serial.print(") is not the same as running version ");
        Serial.println(P_VERSION);
      }
      else
      {
        Serial.print("***UPDATED: Now running version ");
        Serial.println(options.Version());
      }
    }
  }
  // only do update if we are on latest version (ie not some experimental beta)
  String buildtype = options.Version();
  if( buildtype.indexOf("latest") == -1 && buildtype.indexOf("Latest") == -1 && buildtype.indexOf("Local") == -1)
  {
    Serial.print("Update check - using non-'latest' build (");
    Serial.print(buildtype);
    Serial.println(") so aborted.");
    return;
  }
  UpdateURL = options.UpdateServer();
  String VersionURL = UpdateURL.substring(0,UpdateURL.lastIndexOf("/")) + "/version.php";
  Serial.print("Checking for new software update on ");
  Serial.print(VersionURL);

  HTTPClient client;
  client.begin(VersionURL);
  int httpCode = client.GET();
  if (httpCode == 200)
  {
    String payload = client.getString();
    if( strcmp(payload.c_str(), P_VERSION) > 0 )
    {
      UpdateNow = true;
      Serial.print("...YES:");
      Serial.println(payload);
    }
    else
    {
      UpdateNow = false;
      Serial.print("...NO:");
      Serial.println(payload);
    }
  }
}

void TestUpdate()
{
  if( UpdateNow )
  {
    UpdateNow = false;
    Serial.print("Updating using: ");
    Serial.println(UpdateURL);

    HTTPClient client;
    client.begin(UpdateURL);
    int httpCode = client.GET();
    if (httpCode == 200)
    {
        String payload = client.getString();
        UpdateURL = UpdateURL.substring(0,UpdateURL.lastIndexOf("/")) + payload;
        Serial.print("Updating to image: ");
        Serial.println(UpdateURL);
        //int ret = ESPhttpUpdate.update(TheServer.arg(i), 80, payload);
        options.NewVersion(UpdateURL.c_str()); // store the version we're updating to
        Updating = true; // tell the (currently running) code that we are updating.
        ESPhttpUpdate.rebootOnUpdate(true);
        int ret = ESPhttpUpdate.update(UpdateURL);
        Serial.println(ret);
        switch(ret)
        {
          case HTTP_UPDATE_FAILED:
            Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s",  ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
            break;
          case HTTP_UPDATE_NO_UPDATES:
            Serial.println("HTTP_UPDATE_NO_UPDATES");
            break;
          case HTTP_UPDATE_OK:
            Serial.println("HTTP_UPDATE_OK");
            break;
        }
    }
    else
    {
      Serial.print("HTTP error: ");
      Serial.print(client.getString());
      Serial.print(":");
      Serial.println(httpCode);
    }
  }
}
