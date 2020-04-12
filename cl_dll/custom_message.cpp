
//
// custom_message.cpp
//
// implementation of CHudCustomMsg class
//
// Coded By: Pongles

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string.h>
#include <stdio.h>

#define MSG_PICKUP      1
#define MSG_GAME         2
#define MSG_FIRSTBLOOD   3
#define MSG_NAME         4
#define MSG_VICTIM       5
#define MSG_KILLER       6
#define MSG_WEAPON       7

CCustomMessage::CCustomMessage(byte rr, byte gg, byte bb, float yy, float fo, float ht,
      float st, char *szt)
{
   r = rr;
   g = gg;
   b = bb;
   y = yy;
   fadeout = fo;
   holdtime = ht;
   time = st;
   strcpy(szText, szt);
}


DECLARE_MESSAGE(m_CustomMsg, CustomMsg);

int CHudCustomMsg::Init(void)
{
   HOOK_MESSAGE(CustomMsg);

   gHUD.AddHudElem(this);

   Reset();

   return 1;
}

int CHudCustomMsg::VidInit(void)
{
   return 1;
}

void CHudCustomMsg::Reset( void )
{
   for(int i = 0; i < maxCustomMessages; i++)
   {
      if (m_pCustomMsgs[i])
         delete m_pCustomMsgs[i];
      m_pCustomMsgs[i] = NULL;
   }
}

CHudCustomMsg::~CHudCustomMsg( )
{
   for(int i = 0; i < maxCustomMessages; i++)
   {
      if(m_pCustomMsgs[i])
      {
         delete m_pCustomMsgs[i];
      }
   }
}

int CHudCustomMsg::Draw(float flTime)
{
   int Index;
   
   bool BeingDrawn = false;

   float factor, endTime, holdTime;

   CCustomMessage *pMessage;

	if (szSprite)
	{
		int sprite = gHUD.GetSpriteIndex(szSprite);
		SPR_Set(sprite, 255,255,255);
		SPR_DrawAdditive( 0, (ScreenWidth/2), (ScreenHeight/2), &gHUD.GetSpriteRect(sprite));
	}

   // loop though 0 - 16
   for ( Index = 0; Index < maxCustomMessages; Index++ )
   {
      // is there one here?
      if ( m_pCustomMsgs[Index] )
      {
         pMessage = m_pCustomMsgs[Index];

         endTime = pMessage->time + pMessage->fadeout
            + pMessage->holdtime;
         holdTime = pMessage->time + pMessage->holdtime;

         BeingDrawn = true;
         
         if ( flTime <= holdTime )
         {
            // hold
            factor = 1;
         }
         else
         {
            // fade out
            factor = 1 - ((flTime - holdTime) / pMessage->fadeout);
			szSprite = NULL;
         }
           
         gHUD.DrawHudString( (ScreenWidth - CenterPos(pMessage->szText)) / 2,
            pMessage->y, ScreenWidth, pMessage->szText, factor * (pMessage->r),
            factor * (pMessage->g), factor * (pMessage->b) );
           
         // finished ?
         if(flTime >= endTime)
         {
            m_pCustomMsgs[Index] = NULL;
            delete pMessage;
         }
      }
   }

   if ( !BeingDrawn )
      m_iFlags &= ~HUD_ACTIVE;

   return 1;
}

int CHudCustomMsg::MsgFunc_CustomMsg(const char*pszName, int iSize, void *pbuf)
{
   BEGIN_READ(pbuf,iSize);

   int x = READ_BYTE();

   // reads string sent from server
   char *szText = READ_STRING();

   if (x == MSG_PICKUP)
   {
	char *szText2 = READ_STRING();
	MessageAdd2( x, gHUD.m_flTime, szText, szText2 );
   }

   MessageAdd( x, gHUD.m_flTime, szText );

   m_iFlags |= HUD_ACTIVE;

   return 1;
}

int CHudCustomMsg::CenterPos( char *szMessage )
{
   int width = 0;

   for (; *szMessage != 0 && *szMessage != '\n'; szMessage++ )
   {
      width += gHUD.m_scrinfo.charWidths[ *szMessage ];
   }

   return width;
}

void CHudCustomMsg::MessageAdd( int type, float time, char *text )
{
   // check if there is an instance already

   char tempBuffer[64];
   
   if(m_pCustomMsgs[type] != NULL)
   {
      delete m_pCustomMsgs[type];
   }

   // add new instance

   switch ( type )
   {
   case MSG_PICKUP:
      m_pCustomMsgs[type] = new CCustomMessage(255, 35, 35, (ScreenHeight / 2)
            + (ScreenHeight / 4),1.5, 5, time, "Press [Use] To Pickup the Weapon/Item");
      break;
   case MSG_GAME:
      m_pCustomMsgs[type] = new CCustomMessage(192, 192, 192, ScreenHeight / 2,
         1.5, 5, time, text);
      break;
   case MSG_FIRSTBLOOD:
      sprintf(tempBuffer, "%s Drew First Blood", text);
      m_pCustomMsgs[type] = new CCustomMessage(255, 0, 0, ScreenHeight / 4.5,
         1.5, 5, time, tempBuffer);
      break;
   case MSG_NAME:
      sprintf(tempBuffer, "Name: %s", text);
      m_pCustomMsgs[type] = new CCustomMessage(0, 255, 0, ScreenHeight / 2
            + (ScreenHeight / 4),1, 1.5, time, tempBuffer);
      break;
   case MSG_VICTIM:
      sprintf(tempBuffer, "You were killed by %s", text);
      m_pCustomMsgs[type] = new CCustomMessage(0, 0, 255, ScreenHeight / 4,
            1.5, 1, time, tempBuffer);
      break;
   case MSG_KILLER:
      sprintf(tempBuffer, "You Killed %s", text);
      m_pCustomMsgs[type] = new CCustomMessage(255, 0, 0, ScreenHeight / 4.5,
            1.5, 1, time, tempBuffer);
      break;
   case MSG_WEAPON:
      sprintf(tempBuffer, "You got a %s", text);
      m_pCustomMsgs[type] = new CCustomMessage(200, 200, 200, ScreenHeight / 2
            + (ScreenHeight / 4.5), 1.5, 5, time, tempBuffer);
      break;
   }
}

void CHudCustomMsg::MessageAdd2( int type, float time, char *text, char *text2 )
{
      m_pCustomMsgs[type] = new CCustomMessage(255, 35, 35, (ScreenHeight / 2)
            + (ScreenHeight / 4),1.5, 5, time, "Press [Use] To Pickup the Weapon/Item");
	  szSprite = text2;
}