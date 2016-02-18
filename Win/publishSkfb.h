#include "ExportPrint.h"
#include<string>

static PublishSkfbData skfbPbdata;
static std::pair<bool, std::string> lastResponse;
static HWND uploadWindow;

class PublishSkfb
{
public:
    PublishSkfb(void);
    ~PublishSkfb(void);
};

void getPublishSkfbData(PublishSkfbData *pEpd);
void setPublishSkfbData(PublishSkfbData *pEpd);
int doPublishSkfb(HINSTANCE hInst,HWND hWnd);
int uploadToSketchfab(HINSTANCE hInst,HWND hWnd);
