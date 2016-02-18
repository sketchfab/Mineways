/*
Copyright (c) 2011, Eric Haines
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "ExportPrint.h"
#include <string>
#include "stdafx.h"
#include "sketchfabUploader.h"
#include "Resource.h"
#include <sstream>
#include <Windowsx.h>

// Sketchfab API field limits
#define SKFB_NAME_LIMIT         48
#define SKFB_DESC_LIMIT         1024
#define SKFB_TAG_LIMIT          39   // 48 but "mineways " is automatically added so - 9
#define SKFB_TOKEN_LIMIT        32
#define SKFB_PASSWORD_LIMIT     64


static PublishSkfbData skfbPbdata;
static std::pair<bool, std::string> lastResponse;
static HWND uploadWindow;

void getPublishSkfbData(PublishSkfbData *pEpd);
void setPublishSkfbData(PublishSkfbData *pEpd);
int uploadToSketchfab(HINSTANCE hInst,HWND hWnd);
int doPublishSkfb(HINSTANCE hInst,HWND hWnd);
INT_PTR CALLBACK managePublishWindow(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK manageUploadWindow(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
HANDLE uploadThreadHandle = NULL;


void getPublishSkfbData(PublishSkfbData *pSkfbpd)
{
    *pSkfbpd = skfbPbdata;
}

void setPublishSkfbData(PublishSkfbData *pSkfbpd)
{
    skfbPbdata = *pSkfbpd;
}

int uploadToSketchfab(HINSTANCE hInst,HWND hWnd)
{
    // Success or failure are handled in the callback function
    DialogBox(hInst,MAKEINTRESOURCE(IDD_UPLOAD_SKFB),hWnd, manageUploadWindow);
    return 0;
}


int doPublishSkfb(HINSTANCE hInst,HWND hWnd)
{
    gOK = 0;
    DialogBox(hInst,MAKEINTRESOURCE(IDD_PUBLISH_SKFB),hWnd, managePublishWindow);
    // did we hit cancel?
    return gOK;
}

DWORD WINAPI thread_func(LPVOID lpParameter)
{
	UNREFERENCED_PARAMETER(lpParameter);
    SketchfabV2Uploader uploader;
    lastResponse = uploader.upload(uploadWindow, skfbPbdata.skfbApiToken, skfbPbdata.skfbFilePath, skfbPbdata.skfbName, skfbPbdata.skfbDescription, skfbPbdata.skfbTags, skfbPbdata.skfbDraft, skfbPbdata.skfbPrivate, skfbPbdata.skfbPassword);
    SendMessage(uploadWindow, SIGNAL_UPLOAD_FINISHED, 100, 0);
    return 0;
}

INT_PTR CALLBACK manageUploadWindow(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
        {
            uploadWindow = hDlg;
            uploadThreadHandle = CreateThread(NULL, 0, thread_func, NULL, 0, 0);
            break;
        }
    case SIGNAL_UPLOAD_FINISHED:
    {
        if(lastResponse.first){ // Upload succeeded
            int retcode= MessageBox(NULL,
                                    _T("Your model has been successfuly uploaded on Sketchfab.\nClick OK to open a tab on the model page"),
                                    _T("Upload successful"),
                                    MB_OKCANCEL | MB_ICONINFORMATION);
            if(retcode == IDOK)
            {
                std::string modelUrl = lastResponse.second;
                wchar_t* wcharModelUrl = new wchar_t[4096];
                MultiByteToWideChar(CP_ACP, 0, modelUrl.c_str(), modelUrl.size() + 1, wcharModelUrl, 4096);
                ShellExecute(NULL, L"open", wcharModelUrl, NULL, NULL, SW_SHOWNORMAL);
            }
        }
        else { // Upload failed
            std::vector<wchar_t> errorMessage(MultiByteToWideChar(CP_ACP, 0, lastResponse.second.c_str(), lastResponse.second.size() + 1, 0, 0));
            MultiByteToWideChar(CP_ACP, 0, lastResponse.second.c_str(), lastResponse.second.size() + 1, &errorMessage[0], errorMessage.size());
            std::wstring errorMessageStr(&errorMessage[0]);
            MessageBox(NULL,
               errorMessageStr.c_str(),
                       L"Upload failed",
                       MB_OKCANCEL | MB_ICONERROR);
        }

        EndDialog(hDlg, (INT_PTR)TRUE);
        return (INT_PTR)TRUE;
        break;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
            case IDC_SKFB_UPLOAD_CANCEL:
                // Terminate thread
                TerminateThread(uploadThreadHandle, 1);
                EndDialog(hDlg, (INT_PTR)FALSE);
                MessageBox(NULL,
                        _T("Upload cancelled by the user"), _T("Upload interrupted"), MB_OK | MB_ICONERROR);
                return (INT_PTR)FALSE;
            }
            break;
    }
    return (INT_PTR)FALSE;
}


INT_PTR CALLBACK managePublishWindow(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        {
            // Init control values (saved from last upload during if any during this sessino)
            SetDlgItemTextA(hDlg,IDC_SKFB_NAME,skfbPbdata.skfbName);
            SetDlgItemTextA(hDlg,IDC_SKFB_TAG,skfbPbdata.skfbTags);
            SetDlgItemTextA(hDlg,IDC_SKFB_DESC,skfbPbdata.skfbDescription);
            SetDlgItemTextA(hDlg,IDC_SKFB_API_TOKEN,skfbPbdata.skfbApiToken);
            SetDlgItemTextA(hDlg,IDC_SKFB_PASSWORDFIELD,skfbPbdata.skfbPassword);
            CheckDlgButton(hDlg,IDC_SKFB_DRAFT,(skfbPbdata.skfbDraft == false));
            CheckDlgButton(hDlg,IDC_SKFB_PRIVATE,skfbPbdata.skfbPrivate);
            Edit_LimitText(GetDlgItem(hDlg, IDC_SKFB_NAME), SKFB_NAME_LIMIT);
            Edit_LimitText(GetDlgItem(hDlg, IDC_SKFB_TAG), SKFB_TAG_LIMIT);
            Edit_LimitText(GetDlgItem(hDlg, IDC_SKFB_DESC), SKFB_DESC_LIMIT);
            Edit_LimitText(GetDlgItem(hDlg, IDC_SKFB_API_TOKEN), SKFB_TOKEN_LIMIT);
            Edit_LimitText(GetDlgItem(hDlg, IDC_SKFB_PASSWORDFIELD), SKFB_PASSWORD_LIMIT);
            EnableWindow(GetDlgItem(hDlg, IDC_SKFB_PASSWORDFIELD), IsDlgButtonChecked(hDlg, IDC_SKFB_PRIVATE));
        }
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
            case IDC_SKFB_CLAIM_TOKEN:
            {
                ShellExecute(NULL, L"open", L"http://sketchfab.com/settings/password",
                    NULL, NULL, SW_SHOWNORMAL);
                break;
            }
            case IDC_SKFB_PRIVATE:
            {
                EnableWindow(GetDlgItem(hDlg, IDC_SKFB_PASSWORDFIELD), IsDlgButtonChecked(hDlg, IDC_SKFB_PRIVATE));
                break;
            }
            case ID_SKFB_PUBLISH:
            {
                gOK = 1;
                PublishSkfbData lepd = skfbPbdata;

                // Get SKFB specific data
                GetDlgItemTextA(hDlg, IDC_SKFB_API_TOKEN, lepd.skfbApiToken, SKFB_TOKEN_LIMIT + 1);
                GetDlgItemTextA(hDlg, IDC_SKFB_NAME, lepd.skfbName, SKFB_NAME_LIMIT + 1);
                GetDlgItemTextA(hDlg, IDC_SKFB_DESC, lepd.skfbDescription, SKFB_DESC_LIMIT + 1);
                GetDlgItemTextA(hDlg, IDC_SKFB_TAG, lepd.skfbTags, SKFB_TAG_LIMIT + 1);
                lepd.skfbDraft = (IsDlgButtonChecked(hDlg, IDC_SKFB_DRAFT) == 0);
                lepd.skfbPrivate = (IsDlgButtonChecked(hDlg, IDC_SKFB_PRIVATE) == 1);
                if (lepd.skfbPrivate){
                    GetDlgItemTextA(hDlg, IDC_SKFB_PASSWORDFIELD, lepd.skfbPassword, 25);
                }
                // Set it in the main skfbdata object
                skfbPbdata = lepd;
            }
            case IDCANCEL:
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            break;
    }
    return (INT_PTR)FALSE;
}
