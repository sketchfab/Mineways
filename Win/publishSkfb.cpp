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

int uploadToSketchfab(wchar_t* wcZip, std::string api_token, std::string name, std::string description, std::string tags)
{
    SketchfabV2Uploader uploader(api_token);
    std::wstring file(wcZip);
    std::string filepath(file.begin(), file.end());
    std::string uid = uploader.upload(filepath, name, description, tags);
    if (!uid.empty()) {
        std::string status = uploader.poll(uid);
        std::cout << "Model '" << filepath << "' uploaded with status: " << status << std::endl;
        if (status == "SUCCEEDED") {
            std::cout << "Check out: " << uploader.model_url(uid) << std::endl;
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
    char changeString[EP_FIELD_LENGTH];
    //char oldString[EP_FIELD_LENGTH];
    //char currentString[EP_FIELD_LENGTH];
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
            CheckDlgButton(hDlg,IDC_MAKE_Z_UP,epd.chkMakeZUp[epd.fileType]);
            CheckDlgButton(hDlg,IDC_CENTER_MODEL,epd.chkCenterModel);
            CheckDlgButton(hDlg,IDC_INDIVIDUAL_BLOCKS,epd.chkIndividualBlocks);
            CheckDlgButton(hDlg,IDC_BIOME,epd.chkBiome);


            CheckDlgButton(hDlg,IDC_RADIO_ROTATE_0,epd.radioRotate0);
            CheckDlgButton(hDlg,IDC_RADIO_ROTATE_90,epd.radioRotate90);
            CheckDlgButton(hDlg,IDC_RADIO_ROTATE_180,epd.radioRotate180);
            CheckDlgButton(hDlg,IDC_RADIO_ROTATE_270,epd.radioRotate270);

            epd.radioScaleToHeight = 1;

            CheckDlgButton(hDlg,IDC_RADIO_SCALE_BY_COST,epd.radioScaleByCost);

            // OBJ options: gray out if OBJ not in use
            if ( epd.fileType == FILE_TYPE_WAVEFRONT_ABS_OBJ || epd.fileType == FILE_TYPE_WAVEFRONT_REL_OBJ )
            {
                CheckDlgButton(hDlg,IDC_MULTIPLE_OBJECTS,epd.chkMultipleObjects);
                CheckDlgButton(hDlg,IDC_MATERIAL_PER_TYPE,epd.chkMultipleObjects?epd.chkMaterialPerType:BST_INDETERMINATE);
                CheckDlgButton(hDlg,IDC_G3D_MATERIAL,(epd.chkMultipleObjects && epd.chkMaterialPerType)?epd.chkG3DMaterial:BST_INDETERMINATE);
            }
            else
            {
                CheckDlgButton(hDlg,IDC_MULTIPLE_OBJECTS,BST_INDETERMINATE);
                CheckDlgButton(hDlg,IDC_MATERIAL_PER_TYPE,BST_INDETERMINATE);
                CheckDlgButton(hDlg,IDC_G3D_MATERIAL,BST_INDETERMINATE);
            }

            // When handling INITDIALOG message, send the combo box a message:
            for ( int i = 0; i < MTL_COST_TABLE_SIZE; i++ )
                SendDlgItemMessage(hDlg, IDC_COMBO_PHYSICAL_MATERIAL, CB_ADDSTRING, 0, (LPARAM)gMtlCostTable[i].wname);

            SendDlgItemMessage(hDlg, IDC_COMBO_PHYSICAL_MATERIAL, CB_SETCURSEL, epd.comboPhysicalMaterial[epd.fileType], 0);
            prevPhysMaterial = curPhysMaterial = epd.comboPhysicalMaterial[epd.fileType];

            for ( int i = 0; i < MODELS_UNITS_TABLE_SIZE; i++ )
                SendDlgItemMessage(hDlg, IDC_COMBO_MODELS_UNITS, CB_ADDSTRING, 0, (LPARAM)gUnitTypeTable[i].wname);

            SendDlgItemMessage(hDlg, IDC_COMBO_MODELS_UNITS, CB_SETCURSEL, epd.comboModelUnits[epd.fileType], 0);
        }
        return (INT_PTR)TRUE;
    case WM_COMMAND:

        switch (LOWORD(wParam))
        {
        case IDC_RADIO_EXPORT_NO_MATERIALS:
            // set the combo box material to white (might already be that, which is fine)
            SendDlgItemMessage(hDlg, IDC_COMBO_PHYSICAL_MATERIAL, CB_SETCURSEL, PRINT_MATERIAL_WHITE_STRONG_FLEXIBLE, 0);
            // kinda sleazy: if we go to anything but full textures, turn off exporting all objects
            // - done because full blocks of the lesser objects usually looks dumb
            CheckDlgButton(hDlg,IDC_EXPORT_ALL, BST_UNCHECKED);
            goto ChangeMaterial;

        case IDC_RADIO_EXPORT_MTL_COLORS_ONLY:
            // set the combo box material to color (might already be that, which is fine)
            SendDlgItemMessage(hDlg, IDC_COMBO_PHYSICAL_MATERIAL, CB_SETCURSEL, PRINT_MATERIAL_FULL_COLOR_SANDSTONE, 0);
            // kinda sleazy: if we go to anything but full textures, turn off exporting all objects
            CheckDlgButton(hDlg,IDC_EXPORT_ALL, BST_UNCHECKED);
            goto ChangeMaterial;

        case IDC_RADIO_EXPORT_SOLID_TEXTURES:
            // set the combo box material to color (might already be that, which is fine)
            SendDlgItemMessage(hDlg, IDC_COMBO_PHYSICAL_MATERIAL, CB_SETCURSEL, PRINT_MATERIAL_FULL_COLOR_SANDSTONE, 0);
            // kinda sleazy: if we go to anything but full textures, turn off exporting all objects
            CheckDlgButton(hDlg,IDC_EXPORT_ALL, BST_UNCHECKED);
            goto ChangeMaterial;

        case IDC_RADIO_EXPORT_FULL_TEXTURES:
            // set the combo box material to color (might already be that, which is fine)
            SendDlgItemMessage(hDlg, IDC_COMBO_PHYSICAL_MATERIAL, CB_SETCURSEL, PRINT_MATERIAL_FULL_COLOR_SANDSTONE, 0);
            goto ChangeMaterial;

        case IDC_COMBO_PHYSICAL_MATERIAL:
ChangeMaterial:
            {
                // combo box selection will change the thickness, if previous value is set to the default
                curPhysMaterial = (int)SendDlgItemMessage(hDlg, IDC_COMBO_PHYSICAL_MATERIAL, CB_GETCURSEL, 0, 0);
                if ( prevPhysMaterial != curPhysMaterial )
                {
                    //sprintf_s(oldString,EP_FIELD_LENGTH,"%g",METERS_TO_MM * mtlCostTable[prevPhysMaterial].minWall);
                    sprintf_s(changeString,EP_FIELD_LENGTH,"%g",METERS_TO_MM * gMtlCostTable[curPhysMaterial].minWall);

                    // this old code cleverly changed the value only if the user hadn't set it to something else. This
                    // is a little too clever: if the user set the value, then there was no way he could find out what
                    // a material's minimum thickness had to be when he chose the material - he'd have to restart the
                    // program. Better to force the user to set block size again if he changes the material type.
                    //GetDlgItemTextA(hDlg,IDC_BLOCK_SIZE,currentString,EP_FIELD_LENGTH);
                    //if ( strcmp(oldString,currentString) == 0)
                    SetDlgItemTextA(hDlg,IDC_BLOCK_SIZE,changeString);

                    //GetDlgItemTextA(hDlg,IDC_HOLLOW_THICKNESS,currentString,EP_FIELD_LENGTH);
                    //if ( strcmp(oldString,currentString) == 0)
                    SetDlgItemTextA(hDlg,IDC_HOLLOW_THICKNESS,changeString);

                    prevPhysMaterial = curPhysMaterial;
                }

                // if material output turned off, don't allow debug options
                BOOL colorAvailable = !IsDlgButtonChecked(hDlg,IDC_RADIO_EXPORT_NO_MATERIALS)
                    && (epd.fileType != FILE_TYPE_ASCII_STL);
                if ( colorAvailable )
                {
                    // wipe out any indeterminates
                    if ( IsDlgButtonChecked(hDlg,IDC_SHOW_PARTS) == BST_INDETERMINATE )
                    {
                        // back to state at start
                        CheckDlgButton(hDlg,IDC_SHOW_PARTS,epd.chkShowParts);
                        CheckDlgButton(hDlg,IDC_SHOW_WELDS,epd.chkShowParts);
                    }
                }
                else
                {
                    // shut them down
                    CheckDlgButton(hDlg,IDC_SHOW_PARTS,BST_INDETERMINATE);
                    CheckDlgButton(hDlg,IDC_SHOW_WELDS,BST_INDETERMINATE);
                }
                // disallow biome color if not full texture
                CheckDlgButton(hDlg,IDC_BIOME,IsDlgButtonChecked(hDlg,IDC_RADIO_EXPORT_FULL_TEXTURES)?epd.chkBiome:BST_INDETERMINATE);
            }
            break;
        case IDC_SHOW_PARTS:
        case IDC_SHOW_WELDS:
            {
                UINT isInactive = IsDlgButtonChecked(hDlg,IDC_RADIO_EXPORT_NO_MATERIALS)
                    || (epd.fileType == FILE_TYPE_ASCII_STL);
                if ( isInactive )
                {
                    CheckDlgButton(hDlg,IDC_SHOW_WELDS,BST_INDETERMINATE);
                }
                else
                {
                    UINT isIndeterminate = ( IsDlgButtonChecked(hDlg,IDC_SHOW_WELDS) == BST_INDETERMINATE );
                    if ( isIndeterminate )
                        CheckDlgButton(hDlg,IDC_SHOW_WELDS,BST_UNCHECKED);
                }
            }
            break;
        case IDC_MATERIAL_PER_TYPE:
            {
                // it's implied that things are unlocked
                if ( IsDlgButtonChecked(hDlg,IDC_MATERIAL_PER_TYPE) == BST_INDETERMINATE )
                {
                    CheckDlgButton(hDlg,IDC_MATERIAL_PER_TYPE,BST_UNCHECKED);
                }
                if ( IsDlgButtonChecked(hDlg,IDC_MATERIAL_PER_TYPE) )
                {
                    // checked, so the box below becomes active
                    CheckDlgButton(hDlg,IDC_G3D_MATERIAL,epd.chkG3DMaterial);
                }
                else
                {
                    // unchecked, so shut the sub-button down, storing its state for later restoration if turned back on
                    epd.chkG3DMaterial = IsDlgButtonChecked(hDlg,IDC_G3D_MATERIAL);
                    CheckDlgButton(hDlg,IDC_G3D_MATERIAL,BST_INDETERMINATE);
                }
            }
            break;
        case IDC_EXPORT_ALL:
                // if lesser is toggled back off, turn on the defaults
                CheckDlgButton(hDlg,IDC_DELETE_FLOATERS,BST_CHECKED);
                CheckDlgButton(hDlg,IDC_CONNECT_PARTS,BST_CHECKED);
                CheckDlgButton(hDlg,IDC_CONNECT_CORNER_TIPS,BST_CHECKED);
                CheckDlgButton(hDlg,IDC_CONNECT_ALL_EDGES,BST_UNCHECKED);

        case IDC_FATTEN:
            {
                UINT isLesserChecked = IsDlgButtonChecked(hDlg,IDC_EXPORT_ALL);
                if ( !isLesserChecked )
                {
                    CheckDlgButton(hDlg,IDC_FATTEN,BST_INDETERMINATE);
                }
                else
                {
                    UINT isFattenIndeterminate = ( IsDlgButtonChecked(hDlg,IDC_FATTEN) == BST_INDETERMINATE );
                    if ( isFattenIndeterminate )
                        CheckDlgButton(hDlg,IDC_FATTEN,BST_UNCHECKED);
                }
            }
            break;

        case IDC_BLOCK_SIZE:
            // a bit sleazy: if we get focus, then get that the box is changing, change radio button to that choice.
            // There's probably a good way to do this, but I don't know it.
            // The problem is EN_CHANGE happens when IDC_BLOCK_SIZE is first set, and we don't want to do this then
            if ( HIWORD(wParam) == EN_SETFOCUS )
            {
                focus = IDC_BLOCK_SIZE;
            }
            else if ( (HIWORD(wParam) == EN_CHANGE) && (focus == IDC_BLOCK_SIZE) )
            {
                epd.radioScaleToHeight = epd.radioScaleToMaterial = epd.radioScaleByCost = 0;
                epd.radioScaleByBlock = 1;
                CheckDlgButton(hDlg,IDC_RADIO_SCALE_TO_HEIGHT,epd.radioScaleToHeight);
                CheckDlgButton(hDlg,IDC_RADIO_SCALE_TO_MATERIAL,epd.radioScaleToMaterial);
                CheckDlgButton(hDlg,IDC_RADIO_SCALE_BY_BLOCK,epd.radioScaleByBlock);
                CheckDlgButton(hDlg,IDC_RADIO_SCALE_BY_COST,epd.radioScaleByCost);
            }
            break;

        case IDC_FLOAT_COUNT:
            // a bit sleazy: if we get focus, then get that the box is changing, change check button to that choice.
            // There's probably a good way to do this, but I don't know it.
            // The problem is EN_CHANGE happens when IDC_FLOAT_COUNT is first set, and we don't want to do this then
            if ( HIWORD(wParam) == EN_SETFOCUS )
            {
                focus = IDC_FLOAT_COUNT;
            }
            else if ( (HIWORD(wParam) == EN_CHANGE) && (focus == IDC_FLOAT_COUNT) )
            {
                epd.chkDeleteFloaters = 1;
                CheckDlgButton(hDlg,IDC_DELETE_FLOATERS,epd.chkDeleteFloaters);
            }
            break;

        case IDOK:
            {
                gOK = 1;
                ExportFileData lepd;
                lepd = epd;

                // suck all the data out to a local copy
                GetDlgItemTextA(hDlg,IDC_WORLD_MIN_X,lepd.minxString,EP_FIELD_LENGTH);
                GetDlgItemTextA(hDlg,IDC_WORLD_MIN_Y,lepd.minyString,EP_FIELD_LENGTH);
                GetDlgItemTextA(hDlg,IDC_WORLD_MIN_Z,lepd.minzString,EP_FIELD_LENGTH);
                GetDlgItemTextA(hDlg,IDC_WORLD_MAX_X,lepd.maxxString,EP_FIELD_LENGTH);
                GetDlgItemTextA(hDlg,IDC_WORLD_MAX_Y,lepd.maxyString,EP_FIELD_LENGTH);
                GetDlgItemTextA(hDlg,IDC_WORLD_MAX_Z,lepd.maxzString,EP_FIELD_LENGTH);

                lepd.chkCreateZip[lepd.fileType] = IsDlgButtonChecked(hDlg,IDC_CREATE_ZIP);
                lepd.chkCreateModelFiles[lepd.fileType] = IsDlgButtonChecked(hDlg,IDC_CREATE_FILES);

                lepd.radioExportNoMaterials[lepd.fileType] = IsDlgButtonChecked(hDlg,IDC_RADIO_EXPORT_NO_MATERIALS);
                lepd.radioExportMtlColors[lepd.fileType] = IsDlgButtonChecked(hDlg,IDC_RADIO_EXPORT_MTL_COLORS_ONLY);
                lepd.radioExportSolidTexture[lepd.fileType] = IsDlgButtonChecked(hDlg,IDC_RADIO_EXPORT_SOLID_TEXTURES);
                lepd.radioExportFullTexture[lepd.fileType] = IsDlgButtonChecked(hDlg,IDC_RADIO_EXPORT_FULL_TEXTURES);

                //lepd.chkMergeFlattop = IsDlgButtonChecked(hDlg,IDC_MERGE_FLATTOP);
                lepd.chkExportAll = IsDlgButtonChecked(hDlg,IDC_EXPORT_ALL);
                lepd.chkFatten = lepd.chkExportAll?IsDlgButtonChecked(hDlg,IDC_FATTEN) : 0;
                lepd.chkMakeZUp[lepd.fileType] = IsDlgButtonChecked(hDlg,IDC_MAKE_Z_UP);
                lepd.chkCenterModel = IsDlgButtonChecked(hDlg,IDC_CENTER_MODEL);
                lepd.chkIndividualBlocks = IsDlgButtonChecked(hDlg,IDC_INDIVIDUAL_BLOCKS);
                lepd.chkBiome = IsDlgButtonChecked(hDlg,IDC_BIOME);

                lepd.chkFillBubbles = IsDlgButtonChecked(hDlg,IDC_FILL_BUBBLES);
                // if filling bubbles is off, sealing entrances does nothing at all
                lepd.chkSealEntrances = lepd.chkFillBubbles ? IsDlgButtonChecked(hDlg,IDC_SEAL_ENTRANCES) : 0;
                lepd.chkSealSideTunnels = lepd.chkFillBubbles ? IsDlgButtonChecked(hDlg,IDC_SEAL_SIDE_TUNNELS) : 0;

                lepd.chkConnectParts = IsDlgButtonChecked(hDlg,IDC_CONNECT_PARTS);
                // if connect parts is off, corner tips and edges is off
                lepd.chkConnectCornerTips = lepd.chkConnectParts ? IsDlgButtonChecked(hDlg,IDC_CONNECT_CORNER_TIPS) : 0;
                lepd.chkConnectAllEdges = lepd.chkConnectParts ? IsDlgButtonChecked(hDlg,IDC_CONNECT_ALL_EDGES) : 0;

                lepd.chkDeleteFloaters = IsDlgButtonChecked(hDlg,IDC_DELETE_FLOATERS);

                lepd.chkHollow[epd.fileType] = IsDlgButtonChecked(hDlg,IDC_HOLLOW);
                // if hollow is off, superhollow is off
                lepd.chkSuperHollow[epd.fileType] = lepd.chkHollow[epd.fileType] ? IsDlgButtonChecked(hDlg,IDC_SUPER_HOLLOW) : 0;

                lepd.chkMeltSnow = IsDlgButtonChecked(hDlg,IDC_MELT_SNOW);

                BOOL debugAvailable = !lepd.radioExportNoMaterials[lepd.fileType] && (lepd.fileType != FILE_TYPE_ASCII_STL);
                lepd.chkShowParts = debugAvailable ? IsDlgButtonChecked(hDlg,IDC_SHOW_PARTS) : 0;
                lepd.chkShowWelds = debugAvailable ? IsDlgButtonChecked(hDlg,IDC_SHOW_WELDS) : 0;

                lepd.comboPhysicalMaterial[lepd.fileType] = (int)SendDlgItemMessage(hDlg, IDC_COMBO_PHYSICAL_MATERIAL, CB_GETCURSEL, 0, 0);
                lepd.comboModelUnits[lepd.fileType] = (int)SendDlgItemMessage(hDlg, IDC_COMBO_MODELS_UNITS, CB_GETCURSEL, 0, 0);

                GetDlgItemTextA(hDlg,IDC_API_TOKEN,lepd.skfbApiToken,33);
                GetDlgItemTextA(hDlg,IDC_SKFB_NAME,lepd.skfbName,65);
                GetDlgItemTextA(hDlg,IDC_SKFB_DESC,lepd.skfbDescription,1025);
                GetDlgItemTextA(hDlg,IDC_SKFB_TAG,lepd.skfbTags,65);

                // OBJ options
                if ( lepd.fileType == FILE_TYPE_WAVEFRONT_ABS_OBJ || lepd.fileType == FILE_TYPE_WAVEFRONT_REL_OBJ )
                {
                    lepd.chkMultipleObjects = IsDlgButtonChecked(hDlg,IDC_MULTIPLE_OBJECTS);
                    // if filling bubbles is off, sealing entrances does nothing at all
                    if ( lepd.chkMultipleObjects )
                    {
                        // set value only if value above is "unlocked"
                        lepd.chkMaterialPerType = IsDlgButtonChecked(hDlg,IDC_MATERIAL_PER_TYPE);
                        if ( lepd.chkMaterialPerType )
                        {
                            // set value only if value above is "unlocked"
                            lepd.chkG3DMaterial = IsDlgButtonChecked(hDlg,IDC_G3D_MATERIAL);
                        }
                    }
                }

                int nc;
                nc = sscanf_s(lepd.minxString,"%d",&lepd.minxVal);
                nc &= sscanf_s(lepd.minyString,"%d",&lepd.minyVal);
                nc &= sscanf_s(lepd.minzString,"%d",&lepd.minzVal);
                nc &= sscanf_s(lepd.maxxString,"%d",&lepd.maxxVal);
                nc &= sscanf_s(lepd.maxyString,"%d",&lepd.maxyVal);
                nc &= sscanf_s(lepd.maxzString,"%d",&lepd.maxzVal);

                nc &= sscanf_s(lepd.modelHeightString,"%f",&lepd.modelHeightVal);
                nc &= sscanf_s(lepd.blockSizeString,"%f",&lepd.blockSizeVal[lepd.fileType]);
                nc &= sscanf_s(lepd.costString,"%f",&lepd.costVal);

                nc &= sscanf_s(lepd.floaterCountString,"%d",&lepd.floaterCountVal);
                nc &= sscanf_s(lepd.hollowThicknessString,"%g",&lepd.hollowThicknessVal[epd.fileType]);

                // this is a bit lazy checking all errors here, there's probably a better way
                // to test as we go, but this sort of thing should be rare
                if ( nc == 0 )
                {
                    MessageBox(NULL,
                        _T("Bad (non-numeric) value detected in options dialog;\nYou need to clean up, then hit OK again."), _T("Non-numeric value error"), MB_OK|MB_ICONERROR);
                    return (INT_PTR)FALSE;
                }

                if ( lepd.radioScaleToHeight && lepd.modelHeightVal <= 0.0f )
                {
                    MessageBox(NULL,
                        _T("Model height must be a positive number;\nYou need to fix this, then hit OK again."), _T("Value error"), MB_OK|MB_ICONERROR);
                    return (INT_PTR)FALSE;
                }

                if ( lepd.radioScaleByBlock && lepd.blockSizeVal[lepd.fileType] <= 0.0f )
                {
                    MessageBox(NULL,
                        _T("Block size must be a positive number;\nYou need to fix this, then hit OK again."), _T("Value error"), MB_OK|MB_ICONERROR);
                    return (INT_PTR)FALSE;
                }

                if ( lepd.radioScaleByCost )
                {
                    // white vs. colored stuff: $1.50 vs. $3.00 handling fees, plus some minimum amount of material
                    // We need to find out the minimum amount rules for white material; colored is at
                    // http://www.shapeways.com/design-rules/full_color_sandstone, and we use the
                    // "the dimensions have to add up to 65mm" and assume a 3mm block size to give a 59mm*3mm*3mm volume
                    // minimum, times $0.75/cm^3 gives $0.40.
                    if ( lepd.costVal <= (gMtlCostTable[curPhysMaterial].costHandling+gMtlCostTable[curPhysMaterial].costMinimum) )
                    {
                        MessageBox(NULL,
                            _T("The cost must be > $1.55 for colorless, > $3.40 for color;\nYou need to fix this, then hit OK again."), _T("Value error"), MB_OK|MB_ICONERROR);
                        return (INT_PTR)FALSE;
                    }
                }

                if ( lepd.chkDeleteFloaters && lepd.floaterCountVal < 0 )
                {
                    MessageBox(NULL,
                        _T("Floating objects deletion value cannot be negative;\nYou need to fix this, then hit OK again."), _T("Value error"), MB_OK|MB_ICONERROR);
                    return (INT_PTR)FALSE;
                }

                if ( lepd.chkHollow[epd.fileType] && lepd.hollowThicknessVal[epd.fileType] < 0.0 )
                {
                    MessageBox(NULL,
                        _T("Hollow thickness value cannot be negative;\nYou need to fix this, then hit OK again."), _T("Value error"), MB_OK|MB_ICONERROR);
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
