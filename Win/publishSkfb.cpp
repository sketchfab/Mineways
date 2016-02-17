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

#include "stdafx.h"
#include "sketchfabUploader.h"
#include "publishSkfb.h"
#include "Resource.h"
#include <sstream>

#define IS_STL ((epd.fileType == FILE_TYPE_ASCII_STL)||(epd.fileType == FILE_TYPE_BINARY_MAGICS_STL)||(epd.fileType == FILE_TYPE_BINARY_VISCAM_STL))

static int prevPhysMaterial;
static int curPhysMaterial;

PublishSkfb::PublishSkfb(void)
{
}


PublishSkfb::~PublishSkfb(void)
{
}


void getPublishSkfbData(ExportFileData *pEpd)
{
    *pEpd = epd;
}

void setPublishSkfbData(ExportFileData *pEpd)
{
    epd = *pEpd;
}

int uploadToSketchfab(wchar_t* wcZip, HWND progressBar, std::string api_token, std::string name, std::string description, std::string tags, bool draft, bool usePrivate, std::string password)
{
    SketchfabV2Uploader uploader(api_token);
    std::wstring file(wcZip);
    std::string filepath(file.begin(), file.end());
    std::string uid = uploader.upload(progressBar, filepath, name, description, tags, draft, usePrivate, password);
    if (!uid.empty()) {
        std::string status = uploader.poll(uid);
        std::string pp = ("http://sketchfab.com/models/" + uid);
        wchar_t* wString = new wchar_t[4096];
        MultiByteToWideChar(CP_ACP, 0, pp.c_str(), -1, wString, 4096);
        LPCWSTR uu = LPCWSTR(pp.c_str());
        ShellExecute(NULL, L"open", wString,
                    NULL, NULL, SW_SHOWNORMAL);
        if (status == "SUCCEEDED") {
            std::cout << "Check out: " << uploader.model_url(uid) << std::endl;
            return 0;
        }
    }

    return 0;
}

INT_PTR CALLBACK PublishSkfb(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);

int doPublishSkfb(HINSTANCE hInst,HWND hWnd)
{
    gOK = 0;
    DialogBox(hInst,MAKEINTRESOURCE(IDD_PUBLISH_SKFB),hWnd, PublishSkfb);
    // did we hit cancel?
    return gOK;
}

INT_PTR CALLBACK PublishSkfb(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    static int focus = -1;

    switch (message)
    {
    case WM_INITDIALOG:
        {
            sprintf_s(epd.minxString,EP_FIELD_LENGTH,"%d",epd.minxVal);
            sprintf_s(epd.minyString,EP_FIELD_LENGTH,"%d",epd.minyVal);
            sprintf_s(epd.minzString,EP_FIELD_LENGTH,"%d",epd.minzVal);
            sprintf_s(epd.maxxString,EP_FIELD_LENGTH,"%d",epd.maxxVal);
            sprintf_s(epd.maxyString,EP_FIELD_LENGTH,"%d",epd.maxyVal);
            sprintf_s(epd.maxzString,EP_FIELD_LENGTH,"%d",epd.maxzVal);

            SetDlgItemTextA(hDlg,IDC_WORLD_MIN_X,epd.minxString);
            SetDlgItemTextA(hDlg,IDC_WORLD_MIN_Y,epd.minyString);
            SetDlgItemTextA(hDlg,IDC_WORLD_MIN_Z,epd.minzString);
            SetDlgItemTextA(hDlg,IDC_WORLD_MAX_X,epd.maxxString);
            SetDlgItemTextA(hDlg,IDC_WORLD_MAX_Y,epd.maxyString);
            SetDlgItemTextA(hDlg,IDC_WORLD_MAX_Z,epd.maxzString);

            CheckDlgButton(hDlg,IDC_EXPORT_ALL,epd.chkExportAll);
            CheckDlgButton(hDlg,IDC_FATTEN,epd.chkExportAll?epd.chkFatten:BST_INDETERMINATE);
            CheckDlgButton(hDlg,IDC_CENTER_MODEL,epd.chkCenterModel);
            CheckDlgButton(hDlg,IDC_INDIVIDUAL_BLOCKS,epd.chkIndividualBlocks);
            CheckDlgButton(hDlg,IDC_BIOME,epd.chkBiome);

            CheckDlgButton(hDlg,IDC_RADIO_ROTATE_0,epd.radioRotate0);
            CheckDlgButton(hDlg,IDC_RADIO_ROTATE_90,epd.radioRotate90);
            CheckDlgButton(hDlg,IDC_RADIO_ROTATE_180,epd.radioRotate180);
            CheckDlgButton(hDlg,IDC_RADIO_ROTATE_270,epd.radioRotate270);

            epd.radioScaleToHeight = 1;

            // OBJ options: gray out if OBJ not in use
            CheckDlgButton(hDlg,IDC_MULTIPLE_OBJECTS,epd.chkMultipleObjects);
            CheckDlgButton(hDlg,IDC_MATERIAL_PER_TYPE,epd.chkMultipleObjects?epd.chkMaterialPerType:BST_INDETERMINATE);
            CheckDlgButton(hDlg,IDC_G3D_MATERIAL,(epd.chkMultipleObjects && epd.chkMaterialPerType)?epd.chkG3DMaterial:BST_INDETERMINATE);
        }
        return (INT_PTR)TRUE;
    case WM_COMMAND:
    // hypertext pour token (a coté de label) + Limit saisie + limit tags length
    // Publish immediately (enabled) + password(precise optionnal)
    // Popup openon model page + progress bar + remove world coordinates
        switch (LOWORD(wParam))
        {
            case IDC_SKFB_NAME:
            {
                char name[48];
                GetDlgItemTextA(hDlg, IDC_SKFB_NAME, name, 48);
                std::string toto(name);
                std::stringstream ss;
                ss << " " << toto.size() << "/48";
                SetDlgItemTextA(hDlg, IDC_SKFB_NAME_LIMIT, ss.str().c_str());
                break;
            }
            case IDC_SKFB_DESC:
            {
                char description[1026];
                GetDlgItemTextA(hDlg, IDC_SKFB_DESC, description, 1026);
                std::string toto(description);
                std::stringstream ss;
                ss << " " << toto.size() << " / 1024";
                SetDlgItemTextA(hDlg, IDC_SKFB_DESC_LIMIT, ss.str().c_str());
                if (toto.size() > 1024)
                {
                    SetDlgItemTextA(hDlg, IDC_SKFB_DESC_LIMIT, "Description is too long");
                }
                break;
            }
            case IDC_SKFB_CLAIM_TOKEN:
            {
                ShellExecute(NULL, L"open", L"http://sketchfab-local.com/settings/password",
                    NULL, NULL, SW_SHOWNORMAL);
                break;
            }
            case IDC_SKFB_PRIVATE:
            {
                HWND passwordField = GetDlgItem(hDlg, IDC_SKFB_PASSWORDFIELD);
                UINT isChecked = IsDlgButtonChecked(hDlg, IDC_SKFB_PRIVATE);
                if (isChecked){
                    EnableWindow(passwordField, TRUE);
                    CheckDlgButton(hDlg, IDC_SKFB_DRAFT, isChecked ? epd.chkSuperHollow[epd.fileType] : BST_INDETERMINATE);
                }
                else
                {
                    EnableWindow(passwordField, FALSE);
                    CheckDlgButton(hDlg, IDC_SKFB_PRIVATE, isChecked ? epd.chkSuperHollow[epd.fileType] : BST_UNCHECKED);
                }
                break;
            }
            case ID_SKFB_PUBLISH:
            {
                gOK = 1;
                ExportFileData lepd;
                lepd = epd;
                // Copy data settings from UI to export options
                GetDlgItemTextA(hDlg, IDC_WORLD_MIN_X, lepd.minxString, EP_FIELD_LENGTH);
                GetDlgItemTextA(hDlg, IDC_WORLD_MIN_Y, lepd.minyString, EP_FIELD_LENGTH);
                GetDlgItemTextA(hDlg, IDC_WORLD_MIN_Z, lepd.minzString, EP_FIELD_LENGTH);
                GetDlgItemTextA(hDlg, IDC_WORLD_MAX_X, lepd.maxxString, EP_FIELD_LENGTH);
                GetDlgItemTextA(hDlg, IDC_WORLD_MAX_Y, lepd.maxyString, EP_FIELD_LENGTH);
                GetDlgItemTextA(hDlg, IDC_WORLD_MAX_Z, lepd.maxzString, EP_FIELD_LENGTH);

                // Get SKFB specific data
                GetDlgItemTextA(hDlg, IDC_API_TOKEN, lepd.skfbApiToken, 33);
                GetDlgItemTextA(hDlg, IDC_SKFB_NAME, lepd.skfbName, 65);
                GetDlgItemTextA(hDlg, IDC_SKFB_DESC, lepd.skfbDescription, 1025);
                GetDlgItemTextA(hDlg, IDC_SKFB_TAG, lepd.skfbTags, 65);
                lepd.skfbDraft = (IsDlgButtonChecked(hDlg, IDC_SKFB_DRAFT) == 1);
                lepd.skfbPrivate = (IsDlgButtonChecked(hDlg, IDC_SKFB_PRIVATE) == 1);
                if (lepd.skfbPrivate){
                    GetDlgItemTextA(hDlg, IDC_SKFB_PASSWORDFIELD, lepd.skfbPassword, 25);
                }

                // Check inputs
                int nc;
                nc = sscanf_s(lepd.minxString, "%d", &lepd.minxVal);
                nc &= sscanf_s(lepd.minyString, "%d", &lepd.minyVal);
                nc &= sscanf_s(lepd.minzString, "%d", &lepd.minzVal);
                nc &= sscanf_s(lepd.maxxString, "%d", &lepd.maxxVal);
                nc &= sscanf_s(lepd.maxyString, "%d", &lepd.maxyVal);
                nc &= sscanf_s(lepd.maxzString, "%d", &lepd.maxzVal);

                char toto[50];
                GetDlgItemTextA(hDlg, IDC_SKFB_NAME, toto, 50);
                std::string stoto(toto);
                if (stoto.size() == 0)
                {
                    MessageBox(NULL,
                        _T("Please enter a name for your model"), _T("Name error"), MB_OK | MB_ICONERROR);
                    return (INT_PTR)FALSE;
                }

                char desc[1026];
                GetDlgItemTextA(hDlg, IDC_SKFB_DESC, desc, 1026);
                std::string sdesc(desc);
                if (sdesc.size() > 1024)
                {
                    MessageBox(NULL,
                        _T("The model description is too long"), _T("Description error"), MB_OK | MB_ICONERROR);
                    return (INT_PTR)FALSE;
                }
                if ( nc == 0 )
                {
                    MessageBox(NULL,
                        _T("Bad (non-numeric) value detected in options dialog;\nYou need to clean up, then hit OK again."), _T("Non-numeric value error"), MB_OK|MB_ICONERROR);
                    return (INT_PTR)FALSE;
                }
                // survived tests, so really use data
                epd = lepd;
            } // yes, we do want to fall through here
            case IDCANCEL:
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            break;
    }
    return (INT_PTR)FALSE;
}
