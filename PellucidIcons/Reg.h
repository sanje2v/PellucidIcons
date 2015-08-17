/****************************** Module Header ******************************\
Module Name:  Reg.h
Project:      PellucidIcons
Copyright (c) Sanjeev Sharma
Copyright (c) Microsoft Corporation.

The file declares reusable helper functions to register and unregister 
in-process COM components and shell context menu handlers in the registry.


This source is subject to the Microsoft Public License.
See http://www.microsoft.com/opensource/licenses.mspx#Ms-PL.
All other rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, 
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#pragma once

#include <windows.h>


//
//   FUNCTION: RegisterInprocServer
//
//   PURPOSE: Register the in-process component in the registry.
//
//   PARAMETERS:
//   * pszModule - Path of the module that contains the component
//   * clsid - Class ID of the component
//   * pszFriendlyName - Friendly name
//   * pszThreadModel - Threading model
//
//   NOTE: The function creates the HKCR\CLSID\{<CLSID>} key in the registry.
// 
//   HKCR
//   {
//      NoRemove CLSID
//      {
//          ForceRemove {<CLSID>} = s '<Friendly Name>'
//          {
//              InprocServer32 = s '%MODULE%'
//              {
//                  val ThreadingModel = s '<Thread Model>'
//              }
//          }
//      }
//   }
//
HRESULT RegisterInprocServer(PCWSTR pszModule, const CLSID& clsid, 
    PCWSTR pszFriendlyName, PCWSTR pszThreadModel);


//
//   FUNCTION: UnregisterInprocServer
//
//   PURPOSE: Unegister the in-process component in the registry.
//
//   PARAMETERS:
//   * clsid - Class ID of the component
//
//   NOTE: The function deletes the HKCR\CLSID\{<CLSID>} key in the registry.
//
HRESULT UnregisterInprocServer(const CLSID& clsid);


//
//   FUNCTION: RegisterExplorerDesktopContextMenuHandler
//
//   PURPOSE: Register the context menu handler for Explorer Desktop.
//
//   PARAMETERS:
//   * clsid - Class ID of the component
//   * pszFriendlyName - Friendly name
//
//   NOTE: The function creates the following key in the registry.
//
//   HKCR
//   {
//		NoRemove DesktopBackground
//		{
//	         NoRemove shellex
//	         {
//				NoRemove ContextMenuHandlers
//	            {
//					Remove '<Friendly Name>'
//					{
//					    (Default) = s '<CLSID>'
//					}
//	            }
//			 }
//       }
//   }
//
HRESULT RegisterExplorerDesktopContextMenuHandler(const CLSID& clsid, PCWSTR pszFriendlyName);


//
//   FUNCTION: UnregisterExplorerDesktopContextMenuHandler
//
//   PURPOSE: Unregister the context menu handler.
//
//   PARAMETERS:
//   * pszFriendlyName - Friendly name
//
//   NOTE: The function removes the '<Friendly name>' key under 
//   HKCR\DesktopBackground\shellex\ContextMenuHandlers\<Friendly name>
//
HRESULT UnregisterExplorerDesktopContextMenuHandler(PCWSTR pszFriendlyName);

//
//	 FUNCTION: RegisterExplorerIconOverlayHandler
//
//	 PURPOSE: Register icon overlay handler.
//
//	 PARAMETERS:
//	 * pszFriendlyName - Friendly name
//
//   NOTE: The function creates the following key in the registry.
//
//   HKLM
//   {
//      NoRemove SOFTWARE
//      {
//			NoRemove Microsoft
//			{
//	         	NoRemove Windows
//	         	{
//	                NoRemove CurrentVersion
//	                {
//						NoRemove Explorer
//						{
//		                    NoRemove ShellIconOverlayIdentifiers
//							{
//								ForceRemove '<Friendly Name>'
//								{
//									(Default) = s '<CLSID>'
//								}
//							}
//						}
//	                }
//	            }
//          }
//      }
//   }
//
HRESULT RegisterExplorerIconOverlayHandler(const CLSID& clsid, PCWSTR pszFriendlyName);

//
//   FUNCTION: UnregisterExplorerIconOverlayHandler
//
//   PURPOSE: Unregister icon overlay handler.
//
//   PARAMETERS:
//   * pszFriendlyName - Friendly name
//
//   NOTE: The function removes the '<Friendly name>' key under 
//   HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\ShellIconOverlayIdentifiers\<Friendly name>
//
HRESULT UnregisterExplorerIconOverlayHandler(PCWSTR pszFriendlyName);