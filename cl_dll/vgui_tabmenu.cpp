#include "hud.h"
#include "cl_util.h"
#include "vgui_TeamFortressViewport.h"

int centerb = (ScreenHeight/2);
int buttondist = 70;

CTABMenuPanel :: CTABMenuPanel(int iTrans, int iRemoveMe, int x, int y, int wide, int tall) : CMenuPanel(iTrans, iRemoveMe, x,y,wide,tall)
{
    m_pCancelButton = new CommandButton( gHUD.m_TextMessage.BufferedLocaliseTextString( "Cancel" ), 5, centerb+(buttondist*4), XRES(155), YRES(30));
    m_pCancelButton->setParent( this );
    m_pCancelButton->addActionSignal( new CMenuHandler_TextWindow(HIDE_TEXTWINDOW) );

	m_pLoadButton = new CommandButton( gHUD.m_TextMessage.BufferedLocaliseTextString( "Load Game" ), 5, centerb+(buttondist*2), XRES(155), YRES(30));
    m_pLoadButton->setParent( this );
    m_pLoadButton->addActionSignal( new CMenuHandler_StringCommand( "load quick", true ) );

	m_pSaveButton = new CommandButton( gHUD.m_TextMessage.BufferedLocaliseTextString( "Save Game" ), 5, centerb+(buttondist*3), XRES(155), YRES(30));
    m_pSaveButton->setParent( this );
    m_pSaveButton->addActionSignal( new CMenuHandler_StringCommand( "save quick", true ) );

	m_pInfoButton = new CommandButton( gHUD.m_TextMessage.BufferedLocaliseTextString( "Personal Data" ), 5, centerb+(buttondist*1), XRES(155), YRES(30));
    m_pInfoButton->setParent( this );
    m_pInfoButton->addActionSignal( new CMenuHandler_StringCommand( "echo Later.", true ) );
}