//	 									MultiSpec
//
//					Laboratory for Applications of Remote Sensing
//									Purdue University
//								West Lafayette, IN 47907
//								 Copyright (1988-2018)
//							  (c) Purdue Research Foundation
//									All rights reserved.
//
//	File:						SImageOverlays.cpp
//
//	Authors:					Larry L. Biehl
//
//	Revision date:			09/17/2018
//
//	Language:				C
//
//	System:					Linux, Macintosh, and Windows Operating Systems
//
//	Brief description:	The routines in this file handle retrieving information from
//								the window information structure.
//
//	Functions in file:	
//
//	Include files:			"MultiSpecHeaders"
//								"SMultiSpec.h"
//
/* Template for debugging
		int numberChars = sprintf ((char*)gTextString3,
												" SImageOverlays.cpp::xxx (entered routine. %s", 
												gEndOfLine);
		ListString ((char*)gTextString3, numberChars, gOutputTextH);	
*/
//------------------------------------------------------------------------------------

#include "SMultiSpec.h"
	
#if defined multispec_mac || defined multispec_mac_swift
  	#define	IDS_Alert134					134
#endif	// defined multispec_mac || defined multispec_mac_swift    

#if defined multispec_win
	#include "WImageView.h"
#endif	// defined multispec_win

#if defined multispec_lin
	#include "LImageView.h"
#endif	// defined multispec_lin

//#include "SExtGlob.h" 


			// Prototypes for routines in this file that are only called by		
			// other routines in this file.

SInt16 							GetWindowImageOverlayIndex (
										WindowInfoPtr						windowInfoPtr,
										SInt16								imageOverlayIndex);

Handle 							GetImageOverlayInfoMemory (
										SInt16*								overlayIndexPtr);

SInt16 							GetOverlayOffscreenGWorld (
										UInt32								numberClasses, 
										UInt32								columnStart,
										UInt32								columnEnd,
										UInt32								lineStart,
										UInt32								lineEnd,
										SInt16								thematicPaletteType,
										ImageOverlayInfoPtr				imageOverlayInfoPtr); 

void								ReleaseImageOverlayStructureMemory (
										ImageOverlayInfoPtr				imageOverlayInfoPtr);

typedef struct UInt8ColorTable
	{
	UInt8					alpha;
	UInt8					red;
	UInt8					green;
	UInt8					blue;

	} UInt8ColorTable, *UInt8ColorTablePtr;		



//------------------------------------------------------------------------------------
//								 Copyright (1988-2018)
//								(c) Purdue Research Foundation
//									All rights reserved.
//
//	Function name:		void ClearImageOverlay
//
//	Software purpose:	This routine closes the image overlay file as requested from the
//							selected clear image overlay submenu item.
//
//	Parameters in:		menu item index
//
//	Parameters out:	None
//
//	Value Returned:	None
//
// Called By:			
//
//	Coded By:			Larry L. Biehl			Date: 07/13/2009
//	Revised By:			Larry L. Biehl			Date: 07/13/2009

void ClearImageOverlay (
				UInt32			 					menuItemIndex)

{
	Handle*								imageOverlayHandlePtr;
	
	SInt32								selectedOverlay;
	
	UInt32								count,
											index;
	
	
	if (menuItemIndex <= gImageOverlayHandleListLength)
		{
		imageOverlayHandlePtr = (Handle*)GetHandlePointer (gImageOverlayHandle);
		
		count = 0;
		selectedOverlay = -1;															
		for (index=0; index<gImageOverlayHandleListLength; index++)
			{
			if (imageOverlayHandlePtr[index] != NULL)
				{
				count++;
				
				if (count == menuItemIndex)
					{
					selectedOverlay = index;
					break;
					
					}	// end "if (count == menuItemIndex)"
					
				}	// end "if (imageOverlayHandlePtr[index] != NULL)"
				
			}	// end "for (index=0; index<gImageOverlayHandleListLength; index++)"
		
		if (selectedOverlay >= 0)
			CloseImageOverlayFile (selectedOverlay);
																	
		}	// end "if (menuItemIndex <= gImageOverlayHandleListLength)"
	
}	// end "ClearImageOverlay"
									


//------------------------------------------------------------------------------------
//								 Copyright (1988-2018)
//								(c) Purdue Research Foundation
//									All rights reserved.
//
//	Function name:		void CloseAllImageOverlayFiles
//
//	Software purpose:	This routine closes all of the image overlay files and releases 
//							the memory associated with them.
//
//	Parameters in:		None
//
//	Parameters out:	None
//
//	Value Returned:	None
//
// Called By:			
//
//	Coded By:			Larry L. Biehl			Date: 01/07/2003
//	Revised By:			Larry L. Biehl			Date: 01/20/2003

void CloseAllImageOverlayFiles (void)

{	
	UInt32								index;
	
					
			// Dispose of the overlays
	
	for (index=0; index<gImageOverlayHandleListLength; index++)
		{
		CloseImageOverlayFile (index);
		
		}	// end "for (index=0; index<gImageOverlayHandleListLength; index++)"
		
	gImageOverlayHandle = UnlockAndDispose (gImageOverlayHandle);
	gImageOverlayHandleListLength = 0;
	gNumberImageOverlayFiles = 0;
	
}	// end "CloseAllImageOverlayFiles"		



//------------------------------------------------------------------------------------
//								 Copyright (1988-2018)
//								(c) Purdue Research Foundation
//									All rights reserved.
//
//	Function name:		void CloseImageOverlayFile
//
//	Software purpose:	
//
//	Parameters in:		file information pointer
//							pointer to header record information
//
//	Parameters out:	None
//
//	Value Returned:	True, if file was read in okay
//							Fale, if file was not read in okay
//
// Called By:			CheckImageHeader in SOpnImag.cpp
//
//	Coded By:			Larry L. Biehl			Date: 01/03/2003
//	Revised By:			Larry L. Biehl			Date: 01/10/2003

void CloseImageOverlayFile (
				UInt32			 					imageOverlayIndex)

{
	Handle*								imageOverlayHandlePtr;
	WindowInfoPtr						windowInfoPtr;
	WindowPtr							windowPtr;
	
	Handle								windowInfoHandle;

	UInt32								overlayListIndex;
	
	SInt16								windowCount,
											windowListIndex;
	
	Boolean								moveFlag;
		
	
	if (imageOverlayIndex < gImageOverlayHandleListLength)
		{				
		imageOverlayHandlePtr = (Handle*)GetHandlePointer (gImageOverlayHandle, kLock);
																	
		if (imageOverlayHandlePtr != NULL)
			{			
			ReleaseImageOverlayStructureMemory (imageOverlayHandlePtr,
																(SInt16)imageOverlayIndex);
			
					// Now search through all of the image windows and removed this
					//	overlay from the window overlay list. If the overlay file was
					// included in the list compact the list. Also if the overlay file
					// was included make sure that the window is invalidated to force
					// a redraw if the overlay was being shown at the time.	
					
			windowCount = 0;
			windowListIndex = kImageWindowStart;
			
			do
				{
				windowPtr = gWindowList[windowListIndex];         
				windowInfoHandle = GetWindowInfoHandle (windowPtr);
				windowInfoPtr = (WindowInfoPtr)GetHandlePointer (windowInfoHandle, kLock);
			
				if (windowInfoPtr != NULL)
					{
					overlayListIndex = 0;
					moveFlag = FALSE;
					while (overlayListIndex < windowInfoPtr->numberImageOverlays)
						{
						if (abs (windowInfoPtr->imageOverlayList[overlayListIndex].index) ==
																			(UInt8)(imageOverlayIndex + 1))
							{
							if (windowInfoPtr->imageOverlayList[overlayListIndex].index > 0)
								InvalidateWindow (windowPtr, 
															kImageFrameArea, 
															FALSE);
								
							moveFlag = TRUE;
							
							}	// end "if (abs (...->imageOverlayList[overlayListIndex]..."
							
						if (moveFlag && overlayListIndex+1 < windowInfoPtr->numberImageOverlays)
							windowInfoPtr->imageOverlayList[overlayListIndex] = 
												windowInfoPtr->imageOverlayList[overlayListIndex+1];
						
						overlayListIndex++;
						
						}	// end "while (overlayListIndex < windowInfoPtr->...)"
						
					if (moveFlag)
						{
						windowInfoPtr->numberImageOverlays--;
						UpdateOverlayControl (windowPtr);
						
						}	// end "if (moveFlag)"
						
					}	// end "if (windowInfoPtr != NULL)"
					
				CheckAndUnlockHandle (windowInfoHandle);
				
				windowListIndex++;
				windowCount++;
															
				}		while (windowCount<gNumberOfIWindows);
				
			gNumberImageOverlayFiles--;
			
			}	// end "if (shapeInfoPtr != NULL)"
			
		CheckAndUnlockHandle (gImageOverlayHandle);
			
		}	// end "if (overlayIndex < gImageOverlayListLength)"
	
}	// end "CloseImageOverlayFile"				



//------------------------------------------------------------------------------------
//								 Copyright (1988-2018)
//								(c) Purdue Research Foundation
//									All rights reserved.
//
//	Function name:		void CopyToOffscreenBuffer
//
//	Software purpose:	The purpose of this routine is to move the requested lines of 
//							thematic information to the correct location in the offscreen 
//							buffer.
//
//	Parameters in:				
//
//	Parameters out:	
//
//	Value Returned:	
//
// Called By:			ClassifyPerPointArea in SClassify.cpp
//							ISODATAClusterPass in SClustID.cpp
//							OnePassClusterAreas in SClustSP.cpp
//							PostEchoClassifier in SEClssfy.cpp
//
//	Coded By:			Larry L. Biehl			Date: 01/16/2003
//	Revised By:			Larry L. Biehl			Date: 05/12/2017

void CopyToOffscreenBuffer (
				FileIOInstructionsPtr			fileIOInstructionsPtr,
				ImageOverlayInfoPtr				imageOverlayInfoPtr,
				SInt16								imageOverlayIndex,
				Handle								activeImageWindowInfoHandle,
				UInt32								line,
				UInt32								columnStart,
				UInt32								columnInterval,
				UInt32								numberSamples,
				UInt32								lineStart,
				RgnHandle							rgnHandle,
				HUCharPtr							inputBufferPtr,
				HPtr									offScreenBufferPtr,
				SInt32								numberLinesToCopy,
				Boolean								twoByteInputBufferFlag)
				
{ 											
	HPtr									offScreenLineBufferPtr;	
	WindowInfoPtr						imageWindowInfoPtr;
	
	//Handle								activeImageWindowInfoHandle;
	
	SInt16								windowOverlayIndex;
											
																		
			// Get line offscreen buffer pointer

	offScreenLineBufferPtr = GetImageOverlayLineOffscreenPointer (
																				imageOverlayInfoPtr,
																				offScreenBufferPtr,
																				columnStart,
																				line);
	
	FillLineOfOffscreenBuffer (fileIOInstructionsPtr,
										imageOverlayInfoPtr,
										line,
										columnStart,
										columnInterval,
										numberSamples,
										rgnHandle,
										inputBufferPtr,
										offScreenLineBufferPtr,
										twoByteInputBufferFlag);

	if (line == lineStart)
		{
				// Make sure that the overlay can now be drawn.
				
		//activeImageWindowInfoHandle = FindProjectBaseImageWindowInfoHandle ();
			
		imageWindowInfoPtr = (WindowInfoPtr)GetHandlePointer (
																	activeImageWindowInfoHandle);	
								
		if (!gOSXCoreGraphicsFlag)
			imageWindowInfoPtr->drawBaseImageFlag = FALSE;
								
		windowOverlayIndex = GetWindowImageOverlayIndex (imageWindowInfoPtr,
																			imageOverlayIndex);
		
		if (windowOverlayIndex >= 0)								
			imageWindowInfoPtr->imageOverlayList[windowOverlayIndex].index = 
					abs (imageWindowInfoPtr->imageOverlayList[windowOverlayIndex].index);
						
		}	// end "if (line == lineStart)"
	
}	// end "CopyToOffscreenBuffer"



//------------------------------------------------------------------------------------
//								 Copyright (1988-2018)
//								(c) Purdue Research Foundation
//									All rights reserved.
//
//	Function name:		void DrawImageOverlays
//
//	Software purpose:	The purpose of this routine is to draw the offscreen image
// 						overlays in the image window being updated.
//
//	Parameters in:				
//
//	Parameters out:	
//
//	Value Returned:	
//
// Called By:			
//
//	Coded By:			Larry L. Biehl			Date: 01/06/2003
//	Revised By:			Larry L. Biehl			Date: 02/26/2018

void DrawImageOverlays  (
				WindowPtr							windowPtr,
				Handle								windowInfoHandle,
				CGContextRef						context,
				CGRect*								rectanglePtr,
				Rect*									inputWindowRectPtr,
				Rect*									sourceRectPtr,
				SInt16								windowCode)
				
{ 										
	LongRect								overlayDestinationRect;
											
	LCToWindowUnitsVariables		lcToWindowUnitsVariables;
	MapToWindowUnitsVariables		mapToWindowUnitsVariables;
	
	Handle*								imageOverlayHandlePtr;
	ImageOverlayInfoPtr				overlayInfoPtr;
	ImageOverlaySpecs*				overlayListPtr;
	WindowInfoPtr						windowInfoPtr;
	
	SInt32								imageOverlayIndex,
											legendWidth;
	
	UInt32								numberOverlays,
											overlayIndex;
	
	SignedByte							windowHandleStatus;
	/*   
	#if defined multispec_lin
		UInt32                        savedVScroll;
		UInt32                        savedHScroll;
		DisplaySpecsPtr					displaySpecsPtr;
		Handle                        displaySpecsH;
	#endif
	*/									
	#if defined multispec_mac 
		float 								inputHeight;
	#endif		// defined multispec_mac          
	
	
	SetChannelWindowVariables (windowCode, windowInfoHandle, gOSXCoreGraphicsFlag);
		
	windowInfoPtr = (WindowInfoPtr)GetHandleStatusAndPointer (
														windowInfoHandle, &windowHandleStatus);
	
	imageOverlayHandlePtr = NULL;
	if (windowInfoPtr != NULL) 
		{
				// Now get a pointer to the overlay list and the number of overlays.
									
		numberOverlays = windowInfoPtr->numberImageOverlays;
		overlayListPtr = windowInfoPtr->imageOverlayList;
		
		imageOverlayHandlePtr = (Handle*)GetHandlePointer (gImageOverlayHandle, kLock);
														
		}	// end "if (windowInfoPtr != NULL)"
		
	if (imageOverlayHandlePtr != NULL)
		{
				// Set some parameters for converting from map units to window units.
				// 	Set offset for computing closest window units (not for pen location).
				// 	The image top offset is already handled by Core Graphics.
		/*
		#if defined multispec_lin
					// Save the scrolling in displayspecsptr and set it to 1 for linux
			displaySpecsH = GetDisplaySpecsHandle (windowInfoHandle);
			displaySpecsPtr = (DisplaySpecsPtr)GetHandlePointer ();		
			savedVScroll = displaySpecsPtr->origin[kVertical];
			savedHScroll = displaySpecsPtr->origin[kHorizontal];
			displaySpecsPtr->origin[kHorizontal] = 0;
			displaySpecsPtr->origin[kVertical] = 0;
		#endif
		*/				
		SetMapToWindowUnitsVariables (windowInfoHandle,
												windowCode,
												kImageOverlay, 
												gOSXCoreGraphicsFlag, 
												&mapToWindowUnitsVariables);

				// Set the global variables needed to convert from line-column units 
				// to window units.	
				
		SetLCToWindowUnitVariables (windowInfoHandle,
												windowCode, 
												FALSE,
												&lcToWindowUnitsVariables);
		/*      
		#if defined multispec_lin
			displaySpecsPtr->origin[kVertical] = savedVScroll;
			displaySpecsPtr->origin[kHorizontal] = savedHScroll;
		#endif
		*/											
		if (gOSXCoreGraphicsFlag)
			{
			lcToWindowUnitsVariables.magnification = 1.;
			lcToWindowUnitsVariables.xOrigin = 0;
			lcToWindowUnitsVariables.yOrigin = 0;
			lcToWindowUnitsVariables.imageTopOffset = 0;
			legendWidth = lcToWindowUnitsVariables.imageLeftOffset;
									
			#if defined multispec_mac 
				inputHeight = rectanglePtr->size.height;
			#endif		// defined multispec_mac 
			
			}	// end "if (gOSXCoreGraphicsFlag)"
					
		for (overlayIndex=0; overlayIndex<numberOverlays; overlayIndex++)
			{
			imageOverlayIndex = overlayListPtr[overlayIndex].index;
			
			if (imageOverlayIndex > 0)
				{
				imageOverlayIndex--;
				overlayInfoPtr = (ImageOverlayInfoPtr)GetHandlePointer (
															imageOverlayHandlePtr[imageOverlayIndex],
															kLock);
																
				}	// end "if (shapeFileIndex > 0)"
																
			else	// shapeFileIndex < 0
				overlayInfoPtr = NULL;
										
			if (overlayInfoPtr != NULL)
				{																			
				#if defined multispec_mac 
					if (gOSXCoreGraphicsFlag &&
								overlayInfoPtr->offscreenStorageHandle != NULL)
						{
						gCGContextSetAlphaPtr (context, 
														overlayListPtr[overlayIndex].opacity);
														
								// Define the rectangle to draw the image overlay within.
								
						if (overlayInfoPtr->usePlanarCoordinateInfoFlag)
							ConvertMapRectToWinRect (&overlayInfoPtr->boundingMapRectangle, 
																&overlayDestinationRect,
																&mapToWindowUnitsVariables);
							
						else	// !overlayInfoPtr->usePlanarCoordinateInfoFlag
							ConvertLCRectToWinRect (&overlayInfoPtr->lineColumnRect, 
																&overlayDestinationRect, 
																&lcToWindowUnitsVariables);
						
						if (gSideBySideChannels > 1)
							{											
									// If this is a side-by-side channel display then clip to
									// the first channel being displayed.
																
							rectanglePtr->origin.x = overlayDestinationRect.left;
							rectanglePtr->origin.y = (inputHeight -
																	overlayDestinationRect.bottom);
																	
							rectanglePtr->size.width = MIN (
									overlayDestinationRect.right,
									gChannelWindowOffset + gChannelWindowWidth);
							rectanglePtr->size.width -= overlayDestinationRect.left;
									
							rectanglePtr->size.height = (overlayDestinationRect.bottom - 
																	overlayDestinationRect.top);

									// Save a copy of the current graphics state if there 
									// is more than one image overlay so that the clipping 
									// rectangles for the different overlays do not
									// intersect with each other.
							
							if (numberOverlays > 1)
								gCGContextSaveGStatePtr (context);
																	
							gCGContextClipToRectPtr (context, *rectanglePtr);
							
							}	// end "if (gSideBySideChannels > 1)"
																
						rectanglePtr->origin.x = overlayDestinationRect.left - legendWidth;
						rectanglePtr->origin.y = (inputHeight -
																	overlayDestinationRect.bottom);
						rectanglePtr->size.width = (overlayDestinationRect.right -
																	overlayDestinationRect.left);
						rectanglePtr->size.height = (overlayDestinationRect.bottom - 
																	overlayDestinationRect.top);
						
								// Draw the overlay image to the core graphics context.
								
						gCGContextDrawImagePtr (context, 
															*rectanglePtr, 
															overlayInfoPtr->cgInfo.imageRef);
															
						if (gSideBySideChannels > 1 && numberOverlays > 1)
							gCGContextRestoreGStatePtr (context);
						
						}	// if (gOSXCoreGraphicsFlag && ...offscreenStorageHandle != NULL)
				#endif // defined multispec_mac

				#if defined multispec_win     
					DoubleRect							boundingMapRectangle;
					Rect									destinationRect;
					LongRect								intersectWindowRectangle;
	
					BITMAP								transparentBitMap;
					SInt16								bSuccess=0;     		// Success/fail flag
					HDC									hDC;
					//SInt16								titleHeight;
					BLENDFUNCTION						blendFunction;
					Boolean								copyBitsFlag;
					
					
							// Get the bitmap for for HBITMAP
				 
					if (!GetObject ((HBITMAP)overlayInfoPtr->offScreenMapHandle,
											sizeof (BITMAP),
											(LPSTR)&transparentBitMap))
																										return;

					copyBitsFlag = TRUE;

					if (overlayInfoPtr->usePlanarCoordinateInfoFlag)
						{															
								// Define the rectangle to draw the image overlay within.

						ConvertMapRectToWinRect (&overlayInfoPtr->boundingMapRectangle, 
															&overlayDestinationRect,
															&mapToWindowUnitsVariables);

						intersectWindowRectangle.left = MAX (inputWindowRectPtr->left,
																		overlayDestinationRect.left);
						intersectWindowRectangle.top = MAX (inputWindowRectPtr->top,
																		overlayDestinationRect.top);
						intersectWindowRectangle.right = MIN (inputWindowRectPtr->right,
																		overlayDestinationRect.right);
						intersectWindowRectangle.bottom = MIN (inputWindowRectPtr->bottom,
																		overlayDestinationRect.bottom);
						
						if (intersectWindowRectangle.left > intersectWindowRectangle.right ||
								intersectWindowRectangle.top > intersectWindowRectangle.bottom)
							copyBitsFlag = FALSE;
								
						if (copyBitsFlag)
							{				
							destinationRect.top = intersectWindowRectangle.top;
							destinationRect.left = intersectWindowRectangle.left;
							destinationRect.bottom = intersectWindowRectangle.bottom;
							destinationRect.right = intersectWindowRectangle.right;

							ConvertWinRectToMapRect (windowInfoHandle,
																&intersectWindowRectangle,
																&boundingMapRectangle,
																FALSE,
																kUpperLeftLowerRightCorners);

									// Force upper left and lower right for map rectangle.
									
							ConvertMapRectToLCRect (windowInfoHandle,
																&boundingMapRectangle,
																&overlayDestinationRect,
																1);
				
							sourceRectPtr->top = MAX (0, overlayDestinationRect.top -
																	overlayInfoPtr->lineColumnRect.top);
							sourceRectPtr->left = MAX (0, overlayDestinationRect.left -
																	overlayInfoPtr->lineColumnRect.left);
							sourceRectPtr->bottom = MIN (overlayDestinationRect.bottom - 
																	overlayInfoPtr->lineColumnRect.top + 1,
															overlayInfoPtr->lineColumnRect.bottom - 
																	overlayInfoPtr->lineColumnRect.top + 1);
							sourceRectPtr->right = MIN (overlayDestinationRect.right - 
																	overlayInfoPtr->lineColumnRect.left + 1,
															overlayInfoPtr->lineColumnRect.right - 
																	overlayInfoPtr->lineColumnRect.left + 1);
																
							}	// end "if (copyBitsFlag)"
							
						}	// end "if (overlayInfoPtr->usePlanarCoordinateInfoFlag)"
						
					else	// !overlayInfoPtr->usePlanarCoordinateInfoFlag
						{	
						ConvertLCRectToWinRect (&overlayInfoPtr->lineColumnRect, 
															&overlayDestinationRect, 
															&lcToWindowUnitsVariables);
				
						destinationRect.top = overlayDestinationRect.top;
						destinationRect.left = overlayDestinationRect.left;
						destinationRect.bottom = overlayDestinationRect.bottom;
						destinationRect.right = overlayDestinationRect.right;
				
						sourceRectPtr->top = 0;
						//sourceRectPtr->bottom = lpDIBHdr->bmiHeader.biHeight;
						sourceRectPtr->bottom = transparentBitMap.bmHeight;
						sourceRectPtr->left = 0;
						sourceRectPtr->right = transparentBitMap.bmWidth;
						
						}	// end "else !...->usePlanarCoordinateInfoFlag" 
				
					gCDCPointer->SetStretchBltMode (STRETCH_DELETESCANS);
			
					int destinationRectWidth = RECTWIDTH (&destinationRect);
					int destinationRectHeight = RECTHEIGHT (&destinationRect);
					int sourceRectWidth = RECTWIDTH (sourceRectPtr);
					int sourceRectHeight = RECTHEIGHT (sourceRectPtr);

							// Now take into account that the device independant bitmap is
							// is stored in memory in reverse order last line to first line.
							// Learned that this is not needed for AlphaBlend function
							
					//sourceRectPtr->top = 
					//				(int)transparentBitMap.bmHeight - sourceRectPtr->bottom;		
					
					hDC = gCDCPointer->GetSafeHdc ();
			
					if (copyBitsFlag)
						{
						/*
						bSuccess = ::StretchDIBits (
											hDC,            						// hDC
											destinationRect.left,        		// DestX
											destinationRect.top,         		// DestY
											destinationRectWidth,     			// nDestWidth
											destinationRectHeight,    			// nDestHeight
											sourceRectPtr->left,             // SrcX
											sourceRectPtr->top,   				// SrcY
											sourceRectWidth,         			// wSrcWidth
											sourceRectHeight,        			// wSrcHeight
											lpDIBBits,                      	// lpBits
											lpDIBHdr,         					// lpBitsInfo
											DIB_PAL_COLORS,                 	// wUsage
											SRCCOPY);                       	// dwROP (SRCCOPY) (SRCINVERT is close)
						*/
						//sourceHDC = CreateCompatibleDC (hDC);
						//transparentBitMap = CreateDIBSection (
						//		sourceHDC, lpDIBHdr, DIB_RGB_COLORS, &overlayBitsPtr, NULL, 0x0);
						//BlockMoveData (
						//			lpDIBBits, overlayBitsPtr, lpDIBHdr->bmiHeader.biSizeImage);
						//SelectObject (sourceHDC, transparentBitMap);

						blendFunction.BlendOp = AC_SRC_OVER;
						blendFunction.BlendFlags = 0;
						blendFunction.SourceConstantAlpha =
												(UInt8)(overlayListPtr[overlayIndex].opacity*255);
						blendFunction.AlphaFormat = AC_SRC_ALPHA;
						
						bSuccess = ::AlphaBlend (
											hDC,            						// hDC
											destinationRect.left,        		// DestX
											destinationRect.top,         		// DestY
											destinationRectWidth,     			// nDestWidth
											destinationRectHeight,    			// nDestHeight
											overlayInfoPtr->overlayDC,			// source device context
											sourceRectPtr->left,             // SrcX
											sourceRectPtr->top,   				// SrcY
											sourceRectWidth,         			// wSrcWidth
											sourceRectHeight,        			// wSrcHeight
											blendFunction);                  // blendFunction
						
						//DeleteObject (transparentBitMap);
						//DeleteDC (sourceHDC);
						
						/*
						sourceHDC = CreateCompatibleDC (hDC);
						SelectObject (sourceHDC, lpDIBBits);

						bSuccess = ::TransparentBlt (
											hDC,            						// hDC
											destinationRect.left,        		// DestX
											destinationRect.top,         		// DestY
											destinationRectWidth,     			// nDestWidth
											destinationRectHeight,    			// nDestHeight
											sourceHDC,								// source device context
											sourceRectPtr->left,             // SrcX
											sourceRectPtr->top,   				// SrcY
											sourceRectWidth,         			// wSrcWidth
											sourceRectHeight,        			// wSrcHeight
											RGB (0, 0, 0));                    // color which is transparent
						*/
						}	// end "if (copyBitsFlag)"
											
			   	//::GlobalUnlock ((HGLOBAL)overlayInfoPtr->offScreenMapHandle);
			   	//::GlobalUnlock ((HGLOBAL)overlayInfoPtr->offscreenStorageHandle);
				#endif // defined multispec_win 

				#if defined multispec_lin     
					DoubleRect							boundingMapRectangle;
					Rect									destinationRect;
					LongRect								intersectWindowRectangle;

					BITMAPINFOHEADER*					bitMapInfoHeadPtr;
					SInt16								bSuccess = 0; // Success/fail flag
					Boolean								copyBitsFlag;

					bitMapInfoHeadPtr = (BITMAPINFOHEADER*)GetHandlePointer (
														overlayInfoPtr->offScreenMapHandle, kLock);


					copyBitsFlag = TRUE;

					if (overlayInfoPtr->usePlanarCoordinateInfoFlag) 
						{
								// Define the rectangle to draw the image overlay within.
								// In Linux, overlay is drawn on bitmap file so no magnification is required
						double oldmagn = mapToWindowUnitsVariables.magnification;
						mapToWindowUnitsVariables.magnification = 1;
						ConvertMapRectToWinRect (&overlayInfoPtr->boundingMapRectangle,
														  &overlayDestinationRect,
														  &mapToWindowUnitsVariables);
						mapToWindowUnitsVariables.magnification = oldmagn;
						
						intersectWindowRectangle.left = overlayDestinationRect.left;
						intersectWindowRectangle.top = overlayDestinationRect.top;
						intersectWindowRectangle.right = overlayDestinationRect.right;
						intersectWindowRectangle.bottom = overlayDestinationRect.bottom;

						if (intersectWindowRectangle.left > intersectWindowRectangle.right ||
								  intersectWindowRectangle.top > intersectWindowRectangle.bottom)
							copyBitsFlag = FALSE;

						if (copyBitsFlag) 
							{
							destinationRect.top = intersectWindowRectangle.top;
							destinationRect.left = intersectWindowRectangle.left;
							destinationRect.bottom = intersectWindowRectangle.bottom;
							destinationRect.right = intersectWindowRectangle.right;

							ConvertWinRectToMapRect (windowInfoHandle,
															  &intersectWindowRectangle,
															  &boundingMapRectangle,
															  FALSE,
															  kUpperLeftLowerRightCorners);

									// Force upper left and lower right for map rectangle.

							ConvertMapRectToLCRect (windowInfoHandle,
															  &boundingMapRectangle,
															  &overlayDestinationRect,
															  1);

							sourceRectPtr->top = MAX (0, overlayDestinationRect.top -
															overlayInfoPtr->lineColumnRect.top);
							sourceRectPtr->left = MAX (0, overlayDestinationRect.left -
																overlayInfoPtr->lineColumnRect.left);
							sourceRectPtr->bottom = MIN (overlayDestinationRect.bottom -
																	overlayInfoPtr->lineColumnRect.top + 1,
																overlayInfoPtr->lineColumnRect.bottom -
																	overlayInfoPtr->lineColumnRect.top + 1);
							sourceRectPtr->right = MIN (overlayDestinationRect.right -
																  overlayInfoPtr->lineColumnRect.left + 1,
																overlayInfoPtr->lineColumnRect.right -
																  overlayInfoPtr->lineColumnRect.left + 1);

							}	// end "if (copyBitsFlag)"

						}	// end "if (overlayInfoPtr->usePlanarCoordinateInfoFlag)"

					else // !overlayInfoPtr->usePlanarCoordinateInfoFlag
						{
						double oldmagn = lcToWindowUnitsVariables.magnification;
						// Change magnification to 1 for linux
						lcToWindowUnitsVariables.magnification = 1;
						ConvertLCRectToWinRect (&overlayInfoPtr->lineColumnRect,
														  &overlayDestinationRect,
														  &lcToWindowUnitsVariables);
						lcToWindowUnitsVariables.magnification = oldmagn;
						destinationRect.top = overlayDestinationRect.top;
						destinationRect.left = overlayDestinationRect.left;
						destinationRect.bottom = overlayDestinationRect.bottom;
						destinationRect.right = overlayDestinationRect.right;

						sourceRectPtr->top = 0;
						sourceRectPtr->bottom = bitMapInfoHeadPtr->biHeight;
						sourceRectPtr->left = 0;
						sourceRectPtr->right = bitMapInfoHeadPtr->biWidth;

						}	// end "else !...->usePlanarCoordinateInfoFlag" 

					int destinationRectWidth = MAX (
												(destinationRect.right - destinationRect.left), 0);
					int destinationRectHeight = MAX (
												(destinationRect.bottom - destinationRect.top), 0);
					int sourceRectWidth = MAX (
												(sourceRectPtr->right - sourceRectPtr->left), 0);
					int sourceRectHeight = MAX (
												(sourceRectPtr->bottom - sourceRectPtr->top), 0);

							// Now take into account that the device independant bitmap is
							// is stored in memory in reverse order last line to first line.

							// First define a new image buffer data to get rgb values from
							// offscreenstoragehandle. The format of data in
							// offscreenstoragehandle is rgb,alpha.

					int tpixels = destinationRectWidth * destinationRectHeight;
					int pind = 0;
					Handle imagebuffer = malloc (tpixels * 3);
					unsigned char* imagedata =
										(unsigned char*)overlayInfoPtr->offscreenStorageHandle;
					unsigned char* imgbufptr = (unsigned char*)imagebuffer;
					for (pind = 0; pind < tpixels; pind++) 
						{
						unsigned char* imagedataptr = (imagedata + 4*pind);
						*imgbufptr = *(imagedataptr+1);
						*(imgbufptr+1) = *(imagedataptr+2);
						*(imgbufptr+2) = *(imagedataptr + 3);
						imgbufptr = imgbufptr + 3;
						
						}	// end "for (pind = 0; pind < tpixels; pind++)"
					
					wxImage overlayimage (destinationRectWidth,
													destinationRectHeight,
													(unsigned char*)imagebuffer);

					wxBitmap ovbitmap (overlayimage);
					wxMemoryDC overlaydc;
					overlaydc.SelectObject (ovbitmap);

					wxMemoryDC displaydc;
					//bool bitok = (windowPtr->m_ScaledBitmap).IsOk ();
					displaydc.SelectObject (windowPtr->m_ScaledBitmap);

					if (copyBitsFlag) 
						{
						bSuccess = displaydc.Blit (destinationRect.left, // DestX
															  destinationRect.top, // DestY
															  destinationRectWidth, // nDestWidth
															  destinationRectHeight, // nDestHeight)
															  &overlaydc, // Src DC
															  0,//sourceRectPtr->left, // SrcX
															  0); //sourceRectPtr->top // SrcY
													
						}
				#endif // defined multispec_lin 
				
				CheckAndUnlockHandle (imageOverlayHandlePtr[imageOverlayIndex]);
				
				}	// end "if (imageOverlayInfoPtr != NULL)"
				
			}	// end "for (overlayIndex=0; overlayIndex<numberOverlays; ..."
														
		ResetMapToWindowUnitsVariables (windowInfoHandle, &mapToWindowUnitsVariables);
		
		}	// end "if (imageOverlayHandlePtr != NULL)"
		
	MHSetState (windowInfoHandle, windowHandleStatus);

}	// end "DrawImageOverlays"



//------------------------------------------------------------------------------------
//								 Copyright (1988-2018)
//								(c) Purdue Research Foundation
//									All rights reserved.
//
//	Function name:		void FillLineOfOffscreenBuffer
//
//	Software purpose:	The purpose of this routine is to fill a line of the offscreen
//							buffer with the current input cluster information for that
//							line.
//
//	Parameters in:		
//
//	Parameters out:	None
//
// Value Returned:	None			
// 
// Called By:			CopyToOffscreenBuffer in SImageOverlays.cpp
//
//	Coded By:			Larry L. Biehl			Date: 01/06/2003
//	Revised By:			Larry L. Biehl			Date: 05/12/2017	

void FillLineOfOffscreenBuffer (
				FileIOInstructionsPtr			fileIOInstructionsPtr,
				ImageOverlayInfoPtr				imageOverlayInfoPtr,
				SInt32								line,
				UInt32								column,
				UInt32								columnInterval,
				UInt32								numberSamples,
				RgnHandle							rgnHandle,
				HUCharPtr							inputBufferPtr,
				HPtr									offScreenPtr,
				Boolean								twoByteInputBufferFlag)

{	
	HUInt16Ptr							maskBufferPtr,
											twoByteInputBufferPtr;

	SInt32*								colorOffscreenPtr;
	
   Point									point;
	
	SInt16								pointType;
	
	UInt16								maskRequestValue;

	UInt32								colorTableIndex,
											columnEnd,
											sample;
											
	Boolean								includePixelFlag,
											skipLineFlag;

	
	if (offScreenPtr != NULL)
 		{  
		colorOffscreenPtr = (SInt32*)offScreenPtr;
		point.v = (SInt16)line; 
						
		pointType =  gAreaDescription.pointType;	
		skipLineFlag = FALSE;
		if (pointType == kMaskType)
			{
			maskBufferPtr = fileIOInstructionsPtr->maskBufferPtr;
			
			if (*maskBufferPtr == 0)
				skipLineFlag = TRUE;
				
			maskBufferPtr += gAreaDescription.maskColumnStart;
			maskRequestValue = (UInt16)fileIOInstructionsPtr->maskValueRequest;	
			
			columnEnd = gAreaDescription.columnEnd;
			
			}	// end "if (pointType == kMaskType)"
	  		
				// For now 'twoByteInputBufferFlag' implies that this was called from
				// the cluster processor where only those data points that were
				// clustered are in 'inputBufferPtr'.
				
				// For now '!twoByteInputBufferFlag' implies that this was called from
				// the classify processor where all points in the enclosing rectangle
				// of the area are included in 'inputBufferPtr'. 
				
				// This is particularly 
				// important for polygonal type areas. We do want to increment 
				// 'inputBufferPtr' for every pixel in the enclosing rectangle of
				// the area.
				
		twoByteInputBufferPtr = (HUInt16Ptr)inputBufferPtr;
		for (sample=1; sample<=numberSamples; sample++)
			{									
					// Set horizotal (column) point in case it is needed for			
					// polygonal field.	

			point.h = (SInt16)column;
					
			includePixelFlag = FALSE;
			if (pointType == kRectangleType)
				includePixelFlag = TRUE;
				
			else if (pointType == kPolygonType && PtInRgn (point, rgnHandle))
				includePixelFlag = TRUE;
			
			else if (pointType == kMaskType)
				{
						// Note that only the included mask pixels are included in the
						// input line. Step through the columns until an included mask
						// pixel is found.
						
				while (*maskBufferPtr != maskRequestValue && column <= columnEnd)
					{
					colorOffscreenPtr += columnInterval;
					maskBufferPtr += columnInterval;
					column += columnInterval;
					
					}	// end "while (*maskBufferPtr != maskRequestValue)"
					
				if (column <= columnEnd)
					includePixelFlag = TRUE;
				
				}	// end "else if (pointType == kMaskType && ..."
				
			if (includePixelFlag)
				{
				if (twoByteInputBufferFlag)
					{
					colorTableIndex = *twoByteInputBufferPtr % 256;
					*colorOffscreenPtr = 
								imageOverlayInfoPtr->colorTable[colorTableIndex];

					twoByteInputBufferPtr++;
					
					}	// end "if (twoByteInputBufferFlag)"
					
				else	// !twoByteInputBufferFlag
					*colorOffscreenPtr = imageOverlayInfoPtr->colorTable[*inputBufferPtr];
					
				}	// end "if (includePixelFlag)" 
				
			inputBufferPtr++;
				
			colorOffscreenPtr += columnInterval;
			column += columnInterval;
									
			if (pointType == kMaskType)
				maskBufferPtr += columnInterval;
			
			}	// end "for (sample=1; sample<=numberSamples; sample++)"

		}	// end "if (offScreenPtr != NULL)"
		
}	// end "FillLineOfOffscreenBuffer"	



//------------------------------------------------------------------------------------
//								 Copyright (1988-2018)
//								(c) Purdue Research Foundation
//									All rights reserved.
//
//	Function name:		void GetDefaultImageOverlayName
//
//	Software purpose:	This routine returns a default name for the mage
// 						overlay based on the processor that is creating the image 
//							overlay.
//
//	Parameters in:				
//
//	Parameters out:				
//
//	Value Returned:	
// 
// Called By:
//
//	Coded By:			Larry L. Biehl			Date: 01/17/2003
//	Revised By:			Larry L. Biehl			Date: 07/27/2018

void GetDefaultImageOverlayName (
				SInt16								imageOverlayIndex)

{  
	double								thresholdValue;
	
	ImageOverlayInfoPtr				imageOverlayInfoPtr;
	
	FileStringPtr						namePtr;
	
	wchar_t*								wideFileNameCPointer;
	
	Handle								fileInfoHandle;
	
	SInt16								nameLength,
											thresholdPrecision;
	
	SignedByte							fileHandleStatus,
											namePtrIndex,
											overlayHandleStatus;
	
	
	imageOverlayInfoPtr = GetImageOverlayInfoPtr (imageOverlayIndex,
																	kLock,
																	&overlayHandleStatus);
	
	if (imageOverlayInfoPtr != NULL)
		{											
		namePtr = imageOverlayInfoPtr->overlayName;
		namePtr[0] = 0;
		
		fileInfoHandle = FindProjectBaseImageFileInfoHandle ();
		wideFileNameCPointer = (wchar_t*)GetFileNamePPointerFromFileHandle (
																						fileInfoHandle,
																						&fileHandleStatus,
																						kReturnUnicode);
		
		if (wideFileNameCPointer != NULL)
			{
					// Need to allow for unicode characters so that we get
					// 3 full characters.
					
			ConvertUnicodeStringToMultibyteString (wideFileNameCPointer,
																&namePtr[1],
																3,
																&nameLength);
			namePtr[0] = nameLength;
			/*
			numberChars = MIN (fileNamePPointer[0], 3);
			strncpy (&namePtr[1], (char*)&fileNamePPointer[1], numberChars);
			namePtr[0] = (char)numberChars;
			*/
			MHSetState (fileInfoHandle, fileHandleStatus);
			
			}	// end "if (wideFileNameCPointer != NULL)"
		
		thresholdValue = 0.;
		namePtrIndex = namePtr[0] + 1;
		
		if (gProcessorCode == kClusterProcessor && gClusterSpecsPtr != NULL)
			{
			if (gClusterSpecsPtr->mode == 1)
				{
				namePtr[0] += sprintf ((char*)&namePtr[namePtrIndex],
												(char*)"_Clus_SP_%d",
												gClusterSpecsPtr->numberFinalClusters);
				
				}	// end "if (gClusterSpecsPtr->mode == 1)"
				
			else if (gClusterSpecsPtr->mode == 2)
				{
				namePtr[0] += sprintf ((char*)&namePtr[namePtrIndex],
												(char*)"_Clus_ID_%d",
												gClusterSpecsPtr->numberFinalClusters);
				
				}	// end "else if (gClusterSpecsPtr->mode == 2)"
			
			}	// end "if (gProcessorCode == kClusterProcessor && ..."
			
		else if (gProcessorCode == kClassifyProcessor && gClassifySpecsPtr != NULL)
			{
			thresholdPrecision = 1;
			switch (gClassifySpecsPtr->mode)
				{	
				case kMaxLikeMode:		// Gaussian Maximum likelihood 
					namePtr[0] += sprintf ((char*)&namePtr[namePtrIndex],
													"_Clas_Quad_%ld",
													gClassifySpecsPtr->numberClasses);
					thresholdValue = gClassifySpecsPtr->probabilityThreshold;
					break;
					
				case kFisherMode:			// Fisher discriminant
					namePtr[0] += sprintf ((char*)&namePtr[namePtrIndex],
													"_Clas_Fisher_%ld",
													gClassifySpecsPtr->numberClasses);
					thresholdValue = gClassifySpecsPtr->probabilityThreshold;
					break;
					
				case kEuclideanMode:		// Euclidean minimum distance 
					namePtr[0] += sprintf ((char*)&namePtr[namePtrIndex],
													"_Clas_Eucl_%ld",
													gClassifySpecsPtr->numberClasses);
					break;
					
				case kEchoMode:			// Echo
					namePtr[0] += sprintf ((char*)&namePtr[namePtrIndex],
													"_Clas_ECHO_%ld",
													gClassifySpecsPtr->numberClasses);
					thresholdValue = gClassifySpecsPtr->probabilityThreshold;
					break;
					
				case kCorrelationMode:	// Correlation classifier
					namePtr[0] += sprintf ((char*)&namePtr[namePtrIndex],
													"_Clas_Correlation_%ld",
													gClassifySpecsPtr->numberClasses);
					thresholdValue = gClassifySpecsPtr->correlationAngleThreshold;
					break;
					
				case kCEMMode:	// CEM classifier
					namePtr[0] += sprintf ((char*)&namePtr[namePtrIndex],
													"_Clas_CEM_%ld",
													gClassifySpecsPtr->numberClasses);
					thresholdValue = gClassifySpecsPtr->cemThreshold;
					thresholdPrecision = 3;
					break;
					
				case kParallelPipedMode:		// Parallel piped classifer 
					namePtr[0] += sprintf ((char*)&namePtr[namePtrIndex],
													"_Clas_PP_%ld",
													gClassifySpecsPtr->numberClasses);
					break;
					
				}	// end "switch (gClassifySpecsPtr->mode)" 
					
			if (gClassifySpecsPtr->thresholdFlag)
				{
				namePtr[0] += sprintf ((char*)&namePtr[namePtr[0]+1],
												"_%5.*f",
												thresholdPrecision,
												thresholdValue);
				
				}	// end "if (gClassifySpecsPtr->thresholdFlag)"
			
			}	// end "else if (gProcessorCode == kClassifyProcessor)"
			
		if (namePtr[0] == namePtrIndex-1)		// processor not known
			{
			namePtr[0] += sprintf ((char*)&namePtr[namePtrIndex],
											"Image Overlay %d",
											imageOverlayIndex+1);
			
			}	// end "if (namePtr[0] == namePtrIndex-1)"
			
		UnlockImageOverlayInfoHandle (imageOverlayIndex, overlayHandleStatus);
			
		}	// end "if (imageOverlayInfoPtr != NULL)"
		
}	// end "GetDefaultImageOverlayName"	



//------------------------------------------------------------------------------------
//								 Copyright (1988-2018)
//								(c) Purdue Research Foundation
//									All rights reserved.
//
//	Function name:		SInt16 GetImageOverlayIndex
//
//	Software purpose:	The purpose of this routine is to obtain the overlay index
//							for the input window and window image overlay index.
//
//	Parameters in:				
//
//	Parameters out:	
//
//	Value Returned:	
//
// Called By:			
//
//	Coded By:			Larry L. Biehl			Date: 01/20/2003
//	Revised By:			Larry L. Biehl			Date: 04/04/2003

SInt16 GetImageOverlayIndex (
				Handle								windowInfoHandle,
				SInt16								windowOverlayIndex)
				
{ 	
	ImageOverlaySpecs*				overlayListPtr;
	WindowInfoPtr						windowInfoPtr;
											
	SInt16								imageOverlayIndex = -1;
	
	
	windowInfoPtr = (WindowInfoPtr)GetHandlePointer (windowInfoHandle);
	if (windowInfoPtr != NULL && windowOverlayIndex >= 0)
		{
		if (windowOverlayIndex < (SInt16)windowInfoPtr->numberImageOverlays)
			{
			overlayListPtr = windowInfoPtr->imageOverlayList;
			imageOverlayIndex = abs (overlayListPtr[windowOverlayIndex].index) - 1;
			
			}	// end "if (windowOverlayIndex < windowInfoPtr->numberImageOverlays)"
			
		}	// end "if (windowInfoPtr != NULL && ...)"
		
	return (imageOverlayIndex);
	
}	// end "GetImageOverlayIndex"	



//------------------------------------------------------------------------------------
//								 Copyright (1988-2018)
//								(c) Purdue Research Foundation
//									All rights reserved.
//
//	Function name:		Handle GetImageOverlayInfoMemory
//
//	Software purpose:	This routine obtains the memory for the image overlay structure 
//							and stores this memory at the requested image overlay list index
//							or adds it to the end of the list. The index to the structure
//							within the image overlay list is returned.
//
//	Parameters in:				
//
//	Parameters out:				
//
//	Value Returned:	
// 
// Called By:
//
//	Coded By:			Larry L. Biehl			Date: 01/20/2003
//	Revised By:			Larry L. Biehl			Date: 01/20/2003			

Handle GetImageOverlayInfoMemory (
				SInt16*								overlayIndexPtr)

{ 
	Handle*								handlePtr = NULL;
	ImageOverlayInfoPtr				imageOverlayInfoPtr;
	
	Handle								imageOverlayHandle = NULL;
	UInt32								index;
	
	Boolean								changedFlag,
											continueFlag = TRUE;
	
	 
			// Get the image overlay structure to be used.
			
	if (*overlayIndexPtr < 0)
		{
				// Increase size of list of image overlay handles
				
		if (gNumberImageOverlayFiles >= gImageOverlayHandleListLength)
			{
			handlePtr = (Handle*)CheckHandleSize (
													&gImageOverlayHandle,
													&continueFlag, 
													&changedFlag, 
													(gNumberImageOverlayFiles+1)*sizeof (Handle));
			
			if (continueFlag)
				{
				handlePtr[gNumberImageOverlayFiles] = NULL;
				gImageOverlayHandleListLength++;
				*overlayIndexPtr = (SInt16)gNumberImageOverlayFiles;
				
				}	// end "if (continueFlag)"	
				
			else	// !continueFlag
				*overlayIndexPtr = -1;
				
			}	// end "if (gNumberImageOverlayFiles >= ..."
		
		}	// end "if (overlayIndex < 0)"
		
	else	// *overlayIndexPtr >= 0
				// Release memory for current image overlay to be ready to read a
				// new one in.
		CloseImageOverlayFile (*overlayIndexPtr);
		
	if (continueFlag && handlePtr == NULL)
		{
		handlePtr = (Handle*)GetHandlePointer (gImageOverlayHandle, kLock);
											
		continueFlag = (handlePtr != NULL);
		
		}	// end "if (continueFlag && handlePtr == NULL)"
		
	if (continueFlag)
		{
				// Now get handle for image overlay structure.
			
		imageOverlayHandle = MNewHandle (sizeof (ImageOverlayInfo));
		imageOverlayInfoPtr = (ImageOverlayInfoPtr)GetHandlePointer (imageOverlayHandle,
																							kLock);
		
		continueFlag = (imageOverlayInfoPtr != NULL); 
	
		if (continueFlag)	
			InitializeImageOverlayInfoStructure (imageOverlayInfoPtr);
		
		}	//  end "if (continueFlag)"
		
			// Now load the image overlay stucture within the image overlay list
			// of handles.		
			
	if (continueFlag)
		{		
				// Load the handle into the structure where requested.
					
		if (*overlayIndexPtr < 0)
			{	
					// Find index in the image overlay handle list that the image overlay
					// info handle can be stored.
			
			for (index=0; index<gImageOverlayHandleListLength; index++)
				{
				if (handlePtr[index]	== NULL)
					{
					*overlayIndexPtr = (SInt16)index;
					break;
					
					}	// end "if (handlePtr[index]	== NULL)"
					
				}	// end "for (index=0; index<gImageOverlayHandleListLength; index++)"
				
			}	// end "else *overlayIndexPtr < 0"
		
		if (*overlayIndexPtr >= 0)
			{				
			handlePtr[*overlayIndexPtr] = imageOverlayHandle;
			gNumberImageOverlayFiles++;
		
			}	// end "if (*overlayIndexPtr >= 0)"
			
		else	// *overlayIndexPtr < 0
			{
			UnlockAndDispose (imageOverlayHandle);
			imageOverlayHandle = NULL;
			
			}	// else *overlayIndexPtr < 0
		
		}	// end "if (continueFlag)"
					
	CheckAndUnlockHandle (gImageOverlayHandle);
		
	return (imageOverlayHandle);
		
}	// end "GetImageOverlayInfoMemory"	



//------------------------------------------------------------------------------------
//								 Copyright (1988-2018)
//								(c) Purdue Research Foundation
//									All rights reserved.
//
//	Function name:		void GetImageOverlayInfoPtr
//
//	Software purpose:	This routine returns the pointer to the requested image 
//							overlay structure.
//
//	Parameters in:				
//
//	Parameters out:				
//
//	Value Returned:	
// 
// Called By:
//
//	Coded By:			Larry L. Biehl			Date: 01/11/2003
//	Revised By:			Larry L. Biehl			Date: 01/20/2003			

ImageOverlayInfoPtr GetImageOverlayInfoPtr (
				SInt16								overlayIndex,
				Boolean								lockFlag,
				SignedByte*							handleStatusPtr)

{  
	Handle*								imageOverlayHandlePtr;
	ImageOverlayInfoPtr				imageOverlayInfoPtr;
	

	imageOverlayInfoPtr = NULL;
	
	if (handleStatusPtr != NULL)
		handleStatusPtr = 0;
		
	if (overlayIndex >= 0 && (UInt32)overlayIndex < gImageOverlayHandleListLength)
		{
		imageOverlayHandlePtr = (Handle*)GetHandlePointer (gImageOverlayHandle);
	
		if (imageOverlayHandlePtr != NULL)
			{
			if (lockFlag)										
				imageOverlayInfoPtr = (ImageOverlayInfoPtr)GetHandleStatusAndPointer (
														imageOverlayHandlePtr[overlayIndex],
														handleStatusPtr);
														
			else	// !lockFlag
				imageOverlayInfoPtr = (ImageOverlayInfoPtr)GetHandlePointer (
															imageOverlayHandlePtr[overlayIndex]);
														
			}	// end "if (imageOverlayHandlePtr != NULL)"
													
		}	// end "if (overlayIndex >= 0 && ..."
		
	return (imageOverlayInfoPtr);
		
}	// end "GetImageOverlayInfoPtr"



//------------------------------------------------------------------------------------
//								 Copyright (1988-2018)
//								(c) Purdue Research Foundation
//									All rights reserved.
//
//	Function name:		SInt16 GetWindowImageOverlayIndex
//
//	Software purpose:	The purpose of this routine is to obtain the overlay list
//							index that matches the input overlay index.
//
//	Parameters in:				
//
//	Parameters out:	
//
//	Value Returned:	
//
// Called By:			
//
//	Coded By:			Larry L. Biehl			Date: 01/20/2003
//	Revised By:			Larry L. Biehl			Date: 01/20/2003

SInt16 GetWindowImageOverlayIndex (
				Handle								windowInfoHandle,
				SInt16								imageOverlayIndex)
				
{ 	
	WindowInfoPtr						windowInfoPtr;
											
	SInt16								windowImageIndex;
	
	
	windowInfoPtr = (WindowInfoPtr)GetHandlePointer (windowInfoHandle);
	
	windowImageIndex = GetWindowImageOverlayIndex (windowInfoPtr, imageOverlayIndex);
		
	return (windowImageIndex);
	
}	// end "GetWindowImageOverlayIndex"	



//------------------------------------------------------------------------------------
//								 Copyright (1988-2018)
//								(c) Purdue Research Foundation
//									All rights reserved.
//
//	Function name:		SInt16 GetWindowImageOverlayIndex
//
//	Software purpose:	The purpose of this routine is to obtain the overlay list
//							index that matches the input overlay index.
//
//	Parameters in:				
//
//	Parameters out:	
//
//	Value Returned:	
//
// Called By:			
//
//	Coded By:			Larry L. Biehl			Date: 01/16/2003
//	Revised By:			Larry L. Biehl			Date: 01/20/2003

SInt16 GetWindowImageOverlayIndex (
				WindowInfoPtr						windowInfoPtr,
				SInt16								imageOverlayIndex)
				
{ 	
	ImageOverlaySpecs*				overlayListPtr;
	
	UInt32								listIndex,
											numberOverlays;
	
	SInt16								returnListIndex = -1;
	
										
	if (windowInfoPtr != NULL)
		{
		numberOverlays = windowInfoPtr->numberImageOverlays;
		overlayListPtr = windowInfoPtr->imageOverlayList;
		
		for (listIndex=0; listIndex<numberOverlays; listIndex++)
			{
			if (imageOverlayIndex == abs (overlayListPtr[listIndex].index)-1)
				{
				returnListIndex = (SInt16)listIndex;
				break;
				
				}	// end "if (imageOverlayIndex == abs (overlayListPtr..."
			
			}	// end "for (listIndex=0; listIndex<numberOverlays; listIndex++)"	
			
		}	// end "if (windowInfoPtr != NULL)"
		
	return (returnListIndex);
	
}	// end "GetWindowImageOverlayIndex"	



//------------------------------------------------------------------------------------
//								 Copyright (1988-2018)
//								(c) Purdue Research Foundation
//									All rights reserved.
//
//	Function name:		HPtr GetImageOverlayOffscreenPointer
//
//	Software purpose:	This routine returns the pointer to the specified image overlay 
//							and the pointer to the specified image overlay structure.
//
//	Parameters in:				
//
//	Parameters out:				
//
//	Value Returned:	
// 
// Called By:
//
//	Coded By:			Larry L. Biehl			Date: 01/06/2003
//	Revised By:			Larry L. Biehl			Date: 09/17/2018			

HPtr GetImageOverlayOffscreenPointer (
				ImageOverlayInfoPtr				imageOverlayInfoPtr)

{  
	HPtr									offScreenBufferPtr;
	
	PixMapHandle						offScreenPixMapH;
	
	
	offScreenBufferPtr = NULL;
	
	if (imageOverlayInfoPtr != NULL)
		{
		#if defined multispec_mac 
			if (gOSXCoreGraphicsFlag)
				{
						// Note that this handle is already locked.
				
				offScreenBufferPtr = (HPtr)GetHandlePointer (
										imageOverlayInfoPtr->offscreenStorageHandle);
			 
				offScreenPixMapH = 
							(PixMapHandle)imageOverlayInfoPtr->offScreenMapHandle;
			
				}	// end "if (gOSXCoreGraphicsFlag)"
				
			else	// !gOSXCoreGraphicsFlag
				{
				offScreenPixMapH = GetPortPixMap (imageOverlayInfoPtr->offscreenGWorld);
				offScreenBufferPtr = GetPixBaseAddr (offScreenPixMapH);
				
				}	// end "else !gOSXCoreGraphicsFlag"
		#endif // defined multispec_mac 
		
		#if defined multispec_lin
			offScreenBufferPtr = (HPtr)GetHandlePointer (
										imageOverlayInfoPtr->offscreenStorageHandle, kLock);
			offScreenPixMapH = (PixMapHandle)imageOverlayInfoPtr->offScreenMapHandle;
		#endif // defined multispec_lin
		
		#if defined multispec_win
			offScreenBufferPtr = (HPtr)imageOverlayInfoPtr->offscreenStorageHandle;
			offScreenPixMapH = (PixMapHandle)imageOverlayInfoPtr->offScreenMapHandle;
		#endif // defined multispec_win
		
		}	// end "if (imageOverlayInfoPtr != NULL)"
	
	return (offScreenBufferPtr);
	
}	// end "GetImageOverlayOffscreenPointer"



//------------------------------------------------------------------------------------
//								 Copyright (1988-2018)
//								(c) Purdue Research Foundation
//									All rights reserved.
//
//	Function name:		HPtr GetImageOverlayLineOffscreenPointer
//
//	Software purpose:	This routine returns the pointer to the specified line in the
// 						offscreen overlay image buffer. If the line does not exist in
//							the buffer, NULL is return.
//
//	Parameters in:				
//
//	Parameters out:				
//
//	Value Returned:	
// 
// Called By:
//
//	Coded By:			Larry L. Biehl			Date: 01/06/2003
//	Revised By:			Larry L. Biehl			Date: 01/11/2003			

HPtr GetImageOverlayLineOffscreenPointer (
				ImageOverlayInfoPtr				imageOverlayInfoPtr,
				HPtr									offScreenBufferPtr,
				UInt32								columnStart,
				UInt32								lineStart)

{  
	HPtr									offScreenLineBufferPtr = NULL;
	
	SInt32								columnOffset,
											lineOffset;

		
	if (offScreenBufferPtr != NULL)
		{		
		#if defined multispec_mac || defined multispec_lin
			lineOffset = (SInt32)(lineStart - imageOverlayInfoPtr->lineColumnRect.top);
		#endif // defined multispec_mac || defined multispec_lin
			
		#if defined multispec_win 
					// Allow for reverse order of lines in Windows bitmap.
			lineOffset = (SInt32)(imageOverlayInfoPtr->lineColumnRect.bottom - lineStart);
		#endif // defined multispec_win

		if (lineOffset >= 0)
			{
			offScreenLineBufferPtr = &offScreenBufferPtr[
													lineOffset * imageOverlayInfoPtr->rowBytes];
													
			columnOffset = (SInt32)(columnStart - 
												imageOverlayInfoPtr->lineColumnRect.left);
												
			if (columnOffset >= 0)
				offScreenLineBufferPtr = &offScreenLineBufferPtr[4*columnOffset];
													
			}	// end "if (lineOffset >= 0)"
		
		}	// end "if (offScreenBufferPtr != NULL)"
		
	return (offScreenLineBufferPtr);
	
}	// end "GetImageOverlayLineOffscreenPointer"



//------------------------------------------------------------------------------------
//								 Copyright (1988-2018)
//								(c) Purdue Research Foundation
//									All rights reserved.
//
//	Function name:		SInt16 GetOverlayOffscreenGWorld
//
//	Software purpose:	The purpose of this routine is to set up an offscreen
//							pix map for those systems which have 32 bit
//							QuickDraw available.
//
//	Parameters in:		None
//
//	Parameters out:	None
//
// Value Returned:	None			
// 
// Called By:			DisplayColorImage
//
//	Coded By:			Larry L. Biehl			Date: 03/29/2002
//	Revised By:			Larry L. Biehl			Date: 09/17/2018

SInt16 GetOverlayOffscreenGWorld (
				UInt32								numberClasses, 
				UInt32								columnStart,
				UInt32								columnEnd,
				UInt32								lineStart,
				UInt32								lineEnd,
				SInt16								thematicPaletteType,
				ImageOverlayInfoPtr				imageOverlayInfoPtr)

{
	LongRect								offscreenRect;
	
	ColorSpec*							colorSpecPtr;
	CTabPtr								cTabPtr; 
	unsigned char*						colorTablePtr;
	
	CTabHandle							tempCTableHandle;
	UInt32								index,
											numberEntries,
											pixRowBytes,
											tableIndex;
												
	UInt16								paletteCode;
   SInt16                        resultCode;
		
	Boolean								continueFlag;
	

			// Set up offscreen rectangle.
			
	offscreenRect.top = 0;
	offscreenRect.left = 0;
	offscreenRect.right = (SInt32)(columnEnd - columnStart + 1);
	offscreenRect.bottom = (SInt32)(lineEnd - lineStart + 1);
		
			// Get color table information
			// Get a temporary color table handle.		
			// Force to the maximum number of classes for now.								
		
	numberEntries = (UInt16)MIN (numberClasses+3, 256);
	tempCTableHandle = (CTabHandle)MNewHandle (
					(SInt32)numberEntries*sizeof (ColorSpec) + sizeof (ColorTable));
	if (tempCTableHandle == NULL)					
																								return (1);
			
			// Now initialize the color table.
	
	cTabPtr = (CTabPtr)GetHandlePointer ((Handle)tempCTableHandle, kLock);
		
	cTabPtr->ctSeed = GetCTSeed ();
	cTabPtr->ctFlags |= 0x4000;
	
	colorSpecPtr = cTabPtr->ctTable;
		
	continueFlag = LoadColorSpecTable (NULL,
													NULL,
													NULL,
													colorSpecPtr,
													NULL,
													1,
													(UInt16)numberEntries,
													(UInt16)numberClasses,
													thematicPaletteType,
													(UInt16)numberClasses,
													kClassDisplay,
													&paletteCode);
													
	if (!continueFlag)
		{
		UnlockAndDispose ((Handle)tempCTableHandle);
		
																								return (1);
		}
	
			// The first color in the table will be white for thresholded.
			// The cluster class numbers start at 1.
			
	colorTablePtr = (unsigned char*)imageOverlayInfoPtr->colorTable;
			
	colorTablePtr[0] = 0xff;
	colorTablePtr[1] = 0xff;
	colorTablePtr[2] = 0xff;
	colorTablePtr[3] = 0xff;
				
	tableIndex = 4;			
	for (index=0; index<numberEntries; index++)
		{
		#if defined multispec_mac
			colorTablePtr[tableIndex] = 0xff;
			colorTablePtr[tableIndex+1] = (colorSpecPtr[index].rgb.red >> 8);
			colorTablePtr[tableIndex+2] = (colorSpecPtr[index].rgb.green >> 8);
			colorTablePtr[tableIndex+3] = (colorSpecPtr[index].rgb.blue >> 8);
		#endif	// defined multispec_mac
		
		#if defined multispec_lin
			colorTablePtr[tableIndex] = 0xff;
			colorTablePtr[tableIndex+1] = colorSpecPtr[index].rgb.red;
			colorTablePtr[tableIndex+2] = colorSpecPtr[index].rgb.green;
			colorTablePtr[tableIndex+3] = colorSpecPtr[index].rgb.blue;
		#endif	// defined multispec_lin

		#if defined multispec_win 
			colorTablePtr[tableIndex] = (colorSpecPtr[index].rgb.blue >> 8);
			colorTablePtr[tableIndex+1] = (colorSpecPtr[index].rgb.green >> 8);
			colorTablePtr[tableIndex+2] = (colorSpecPtr[index].rgb.red >> 8);
			colorTablePtr[tableIndex+3] = 0x7f;		// 0xff
		#endif	// defined multispec_mac 

		tableIndex += 4;
		
		}	// end "for (index=0; index<numberEntries; index++)"
									
	CheckAndUnlockHandle ((Handle)tempCTableHandle);
	
	#if defined multispec_mac 
		SInt64								sInt64FreeBytesNeeded;
		
		Rect									rect;
		
		CGrafPtr								savedPort;
		GWorldPtr							offscreenGWorld; 
		                                         
		GDHandle								savedDevice; 
		Handle								tempHandle;
		PixMapHandle						offScreenPixMapH;  
		
		SInt32								freeBytesNeeded;
		
		//SInt16								resultCode;
		
		
				// Initialize local variables.													
		
		resultCode = noErr;		
		
				// Save current graphics world.													
				
		GetGWorld (&savedPort, &savedDevice);
		
		offscreenGWorld = imageOverlayInfoPtr->offscreenGWorld;
		offScreenPixMapH = (PixMapHandle)imageOverlayInfoPtr->offScreenMapHandle;
		
				// Make certain that the number of pixel row bytes is less than the	
				// maximum allowed.					
						
		pixRowBytes = GetNumberPixRowBytes ((UInt32)offscreenRect.right, 32);
									
		if (pixRowBytes > gMaxRowBytes)
			resultCode = 1; 
		
		sInt64FreeBytesNeeded = (SInt64)pixRowBytes * offscreenRect.bottom;
		
		if (sInt64FreeBytesNeeded > SInt32_MAX)
			resultCode = 1;
		
		if (resultCode == noErr)
			{
			freeBytesNeeded = (SInt32)sInt64FreeBytesNeeded;
		
			if (gOSXCoreGraphicsFlag)
				{
						// If this is from the classify processor, we will need to 0 out the
						// offscreen memory because it is possible for only a portion of the 
						// buffer will be written in.
						// Decided on 2/2/2012 that we always need to clear out the memory because
						// a user might cancel in the middle and could be left with a strange background
						// to understand. Will see how this works for awhile before removed the lines
						// that are commented out.
							
				//if (gProcessorCode == kClassifyProcessor && gClassifySpecsPtr != NULL &&
				//			(gClassifySpecsPtr->trainingFldsResubstitutionFlag ||
				//								gClassifySpecsPtr->trainingFldsLOOFlag ||
				//													gClassifySpecsPtr->testFldsFlag))
				imageOverlayInfoPtr->offscreenStorageHandle = MNewHandleClear (
																						freeBytesNeeded);
				
				if (imageOverlayInfoPtr->offscreenStorageHandle == NULL)
					resultCode = 1;
					
						// Get storage for save a portPixMap in.
				
				if (offScreenPixMapH == NULL)
					offScreenPixMapH = NewPixMap ();
					
				if (offScreenPixMapH == NULL)
					resultCode = 1;
			
				if (resultCode == noErr)
					{
					CGColorSpaceRef		baseColorSpace = NULL,
												colorSpace = NULL;
												
					Ptr						offScreenBufferPtr = NULL;
												
					CGDataProviderRef		provider = NULL;
					CGImageAlphaInfo		alphaInfo;
					size_t					bitsPerComponent;
					
					imageOverlayInfoPtr->offScreenMapHandle = (Handle)offScreenPixMapH;
					imageOverlayInfoPtr->offscreenMapSize = freeBytesNeeded;
					
					imageOverlayInfoPtr->cgInfo.contextRowBytes = pixRowBytes;
					
							// Lock the handle for the offscreen bit map and keep it locked.
						
					offScreenBufferPtr = GetHandlePointer (
													imageOverlayInfoPtr->offscreenStorageHandle, 
													kLock);
								
							// Note that the image base address handle is already locked.
							
					provider = gCGDataProviderCreateWithDataPtr (
													NULL,
													offScreenBufferPtr, 
													freeBytesNeeded, 
													NULL);
						
							// Create an indexed color space for a thematic window.
							
					baseColorSpace = gCGColorSpaceCreateDeviceRGBPtr ();
				
					//alphaInfo = kCGImageAlphaNoneSkipFirst;
					alphaInfo = kCGImageAlphaFirst;
					bitsPerComponent = 8;
							
					imageOverlayInfoPtr->cgInfo.imageRef = gCGImageCreatePtr (
													offscreenRect.right,
													offscreenRect.bottom,
													bitsPerComponent,
													32,
													imageOverlayInfoPtr->cgInfo.contextRowBytes,
													baseColorSpace,
													alphaInfo,
													provider,
													NULL,
													1,
													kCGRenderingIntentDefault);
					/*
					colorSpace = gCGColorSpaceCreateIndexedPtr (
														baseColorSpace, 
														numberEntries-1, 
														tablePtr);
					
							// Create the image.
					
					alphaInfo = kCGImageAlphaNone;
					bitsPerComponent = 8;
							
					imageOverlayInfoPtr->cgInfo.imageRef = gCGImageCreatePtr (
													offscreenRect.right,
													offscreenRect.bottom,
													bitsPerComponent,
													8,
													imageOverlayInfoPtr->cgInfo.contextRowBytes,
													colorSpace,
													alphaInfo,
													provider,
													NULL,
													0,
													kCGRenderingIntentDefault);
					*/
					/*
					float						decode[2];
					decode[0] = 0;	
					decode[1] = 1;	
					bitsPerComponent = 1;
					pixRowBytes = 2*(offscreenRect.right+16)/16;
					imageOverlayInfoPtr->cgInfo.imageRef = gCGImageMaskCreatePtr (
													offscreenRect.right,
													offscreenRect.bottom,
													bitsPerComponent,
													1,
													pixRowBytes,
													provider,
													decode,
													0);
					*/
							// Release the references to the provider and the colorspace. Copies
							// are retained by cgImage.
							
					gCGDataProviderReleasePtr (provider);
					
					gCGColorSpaceReleasePtr (baseColorSpace);
					
					gCGColorSpaceReleasePtr (colorSpace);
						
					(*offScreenPixMapH)->pixelSize = 8;	
						
					}	// end "if (resultCode == noErr)" 
				
				}	// end "if (gOSXCoreGraphicsFlag)"
			
			if (!gOSXCoreGraphicsFlag)
				{
				rect.left = offscreenRect.left;			//  - 32767;
				rect.top = offscreenRect.top;				// - 32767;
				rect.right = offscreenRect.right;		// - 32767;
				rect.bottom = offscreenRect.bottom;		// - 32767;
				
						// Verify that there is enough memory available for the offscreen 
						// GWorld.
					
				tempHandle = MNewHandle (freeBytesNeeded + 10000);
				if (tempHandle == NULL)
					resultCode = 1;
				
				else	// tempHandle != NULL 
					UnlockAndDispose (tempHandle);

				if (resultCode == noErr)
					resultCode = NewGWorld (&offscreenGWorld, 
													32, 		// 8
													&rect, 
													tempCTableHandle, 
													NULL, 
													(SInt32)0);
			
				if (resultCode == noErr)
					{
					imageOverlayInfoPtr->offscreenGWorld = offscreenGWorld;
					imageOverlayInfoPtr->offScreenMapHandle = 
																(Handle)GetPortPixMap (offscreenGWorld);
					
							// Get the actual size of the image buffer, in case that pix row
							// bytes is not exactly as computed above.
							// Make sure that the first two bits are cleared. They are not used
							// for the number of row bytes.
							
					pixRowBytes = MGetPixRowBytes (GetPortPixMap (offscreenGWorld));
					freeBytesNeeded = pixRowBytes * offscreenRect.bottom;
					
					imageOverlayInfoPtr->offscreenMapSize = freeBytesNeeded;
					
							// Lock down the pixels before drawing image into the window.  	
							// Draw only if offscreen buffer is not purged.							
							
					SetGWorld (offscreenGWorld, NULL);
					
					if (LockPixels (GetPortPixMap (offscreenGWorld)))
						EraseRect (&rect);
						
					else	// !LockPixels (offscreenGWorld->portPixMap) 
						resultCode = 1;
						
					}	// end "if (resultCode == noErr)" 

				}	// end "else !gOSXCoreGraphicsFlag"
				
			}	// end "if resultCode == noError
			
		SetGWorld (savedPort, savedDevice);
	#endif	// defined multispec_mac 
	
	UnlockAndDispose ((Handle)tempCTableHandle);

	#if defined multispec_win 
		SInt64								sInt64FreeBytesNeeded;
		
		//BITMAPINFO*							bitMapInfoPtr;
		BITMAPINFO							bitMapInfo;

		BITMAPINFOHEADER*					bitMapInfoHeadPtr;
		void*									bitMapPtr;
		
		UInt32								maxRowBytes,
												numberColumns; 
		

				// Make certain that the number of columns is less than the	
				// maximum allowed.							
						
		numberColumns = (UInt32)offscreenRect.right - offscreenRect.left;
		pixRowBytes = GetNumberPixRowBytes (numberColumns, 32); 
		if (pixRowBytes > gMaxRowBytes)
																							return (1); 
				
				// Now get memory for the offscreen bit map and header. 
				
		resultCode = noErr;     
		//numberEntries = (SInt16)numberClasses;
																							
				// Check if the number of bytes needed for the new BITMAP
				// is the same as that in the current BITMAP; if not release the
				// old block of memory and get a new one.
				// Change on 2/16/2000 so that the old BITMAP was always cleared out.
		
		sInt64FreeBytesNeeded = 
					(SInt64)pixRowBytes * ((SInt64)offscreenRect.bottom - offscreenRect.top);
		
				// It seems like there is a limit of 256,000,000 bytes for the image size
				// for the overlay to work properly. 9/14/2018.

		if (sInt64FreeBytesNeeded > 256000000)
			resultCode = 1;
		
		ReleaseImageOverlayStructureMemory (imageOverlayInfoPtr);

		//if (resultCode == noErr)
			//{
			//bytesNeeded = (SInt32)sInt64FreeBytesNeeded;
			//UnlockAndDispose (imageOverlayInfoPtr->offscreenStorageHandle);
			//imageOverlayInfoPtr->offscreenStorageHandle = MNewHandle (bytesNeeded);
			//imageOverlayInfoPtr->offscreenMapSize = bytesNeeded;
		
			//if (imageOverlayInfoPtr->offscreenStorageHandle == NULL)
			//	resultCode = 1;
			
			//}	// end "if (resultCode == noErr)"
		
				// Dispose of old BITMAPINFOHEADER if it exists and get a new one.	
		
		//imageOverlayInfoPtr->offScreenMapHandle = 
		//							UnlockAndDispose (imageOverlayInfoPtr->offScreenMapHandle);
		/*									
		if (resultCode == noErr)
			{
					// Get BITMAPINFOHEADER
					// Space is always allowed for a 256 entry table.
					
			bytesNeeded = sizeof (BITMAPINFOHEADER) + 256*sizeof (UInt16);
			imageOverlayInfoPtr->offScreenMapHandle = MNewHandle (bytesNeeded);
			
			if (imageOverlayInfoPtr->offScreenMapHandle == NULL)
				resultCode = 1; 
			
			}	// end "if (resultCode == noErr)"
		*/															
		if (resultCode == noErr)
			{
			bitMapInfoHeadPtr = (BITMAPINFOHEADER*)&bitMapInfo;

					// Load info in BITMAPINFOHEADER
					
			//bitMapInfoHeadPtr = (BITMAPINFOHEADER*)GetHandlePointer (
			//									imageOverlayInfoPtr->offScreenMapHandle, kLock); 
														
			bitMapInfoHeadPtr->biSize = 40;		
			bitMapInfoHeadPtr->biWidth = offscreenRect.right - offscreenRect.left;	
			bitMapInfoHeadPtr->biHeight = offscreenRect.bottom - offscreenRect.top;	
			bitMapInfoHeadPtr->biPlanes = 1;	
			bitMapInfoHeadPtr->biBitCount = 32;			// 8
			bitMapInfoHeadPtr->biCompression = BI_RGB;	
			bitMapInfoHeadPtr->biSizeImage = (DWORD)sInt64FreeBytesNeeded;
			bitMapInfoHeadPtr->biXPelsPerMeter = 0;	
			bitMapInfoHeadPtr->biYPelsPerMeter = 0;	
			bitMapInfoHeadPtr->biClrUsed = 0; // numberEntries	
			bitMapInfoHeadPtr->biClrImportant = 0;

			imageOverlayInfoPtr->offscreenMapSize = (UInt32)sInt64FreeBytesNeeded;

			CDC*	pDC = gActiveImageViewCPtr->GetDC ();
			HDC	hDC = pDC->GetSafeHdc ();
			imageOverlayInfoPtr->overlayDC = CreateCompatibleDC (hDC);

			HBITMAP transparentBitMap = CreateDIBSection (imageOverlayInfoPtr->overlayDC,
																			(BITMAPINFO*)bitMapInfoHeadPtr,
																			DIB_RGB_COLORS,
																			&bitMapPtr,
																			NULL,
																			0);

			SelectObject (imageOverlayInfoPtr->overlayDC, transparentBitMap);

			imageOverlayInfoPtr->offScreenMapHandle = (Handle)transparentBitMap;
			imageOverlayInfoPtr->offscreenStorageHandle = (Handle)bitMapPtr;

			if (transparentBitMap == NULL)
				resultCode = 2;
				
			//CheckAndUnlockHandle (imageOverlayInfoPtr->offScreenMapHandle);                                    
				
			}	// end "if (resultCode == noErr)" 
			
		if (resultCode != noErr) 
			{
			ReleaseImageOverlayStructureMemory (imageOverlayInfoPtr);
			/*
			DeleteObject (transparentBitMap);
			DeleteDC (imageOverlayInfoPtr->overlayDC);
			imageOverlayInfoPtr->offScreenMapHandle = NULL;
			imageOverlayInfoPtr->offscreenStorageHandle = NULL;
			imageOverlayInfoPtr->overlayDC = NULL;
			//imageOverlayInfoPtr->offScreenMapHandle =  
			//					UnlockAndDispose (imageOverlayInfoPtr->offScreenMapHandle);
			
			//imageOverlayInfoPtr->offscreenStorageHandle = 							
			//					UnlockAndDispose (imageOverlayInfoPtr->offscreenStorageHandle);
			
			imageOverlayInfoPtr->offscreenMapSize = 0;
			*/
			}	// end "if (resultCode != noErr)"
	#endif	// defined multispec_win 

	#if defined multispec_lin 
		SInt64					sInt64FreeBytesNeeded;
		BITMAPINFOHEADER*		bitMapInfoHeadPtr;

		UInt32					bytesNeeded,
									maxRowBytes;

				// Make certain that the number of pixel row bytes is less than the	
				// maximum allowed.							

		pixRowBytes = GetNumberPixRowBytes (
										  (UInt32) offscreenRect.right - offscreenRect.left,
										  32);

		maxRowBytes = gMaxRowBytes;

		if (pixRowBytes > maxRowBytes)
																								return (1);

				// Now get memory for the offscreen bit map and header. 

		resultCode = noErr;

				// Check if the number of bytes needed for the new BITMAP
				// is the same as that in the current BITMAP; if not release the
				// old block of memory and get a new one.
				// Change on 2/16/2000 so that the old BITMAP was always cleared out.

		sInt64FreeBytesNeeded = (SInt64)pixRowBytes *
										((SInt64)offscreenRect.bottom - offscreenRect.top + 1);

		if (sInt64FreeBytesNeeded > SInt32_MAX)
			resultCode = 1;

		if (resultCode == noErr)
			{
			bytesNeeded = (SInt32)sInt64FreeBytesNeeded;
			UnlockAndDispose (imageOverlayInfoPtr->offscreenStorageHandle);
			imageOverlayInfoPtr->offscreenStorageHandle = MNewHandle (bytesNeeded);
			imageOverlayInfoPtr->offscreenMapSize = bytesNeeded;

			if (imageOverlayInfoPtr->offscreenStorageHandle == NULL)
				resultCode = 1;

			} // end "if (resultCode == noErr)"

				// Dispose of old BITMAPINFOHEADER if it exists and get a new one.	

		imageOverlayInfoPtr->offScreenMapHandle =
								UnlockAndDispose (imageOverlayInfoPtr->offScreenMapHandle);

		if (resultCode == noErr)
			{
					// Get BITMAPINFOHEADER
					// Space is always allowed for a 256 entry table.

			bytesNeeded = sizeof (BITMAPINFOHEADER) + 256 * sizeof (UInt16);
			imageOverlayInfoPtr->offScreenMapHandle = MNewHandle (bytesNeeded);

			if (imageOverlayInfoPtr->offScreenMapHandle == NULL)
				resultCode = 1;

			}	// end "if (resultCode == noErr)"

		if (resultCode == noErr)
			{
			//bitMapInfoHeadPtr = (BITMAPINFOHEADER*)&bitMapInfo;

					// Load info in BITMAPINFOHEADER

			bitMapInfoHeadPtr = (BITMAPINFOHEADER*) GetHandlePointer (
												imageOverlayInfoPtr->offScreenMapHandle, kLock);

			bitMapInfoHeadPtr->biSize = 40;
			bitMapInfoHeadPtr->biWidth = offscreenRect.right - offscreenRect.left;
			bitMapInfoHeadPtr->biHeight = offscreenRect.bottom - offscreenRect.top;
			bitMapInfoHeadPtr->biPlanes = 1;
			bitMapInfoHeadPtr->biBitCount = 32; // 8
			bitMapInfoHeadPtr->biCompression = 0; //BI_RGB;	
			bitMapInfoHeadPtr->biSizeImage = 0;
			bitMapInfoHeadPtr->biXPelsPerMeter = 0;
			bitMapInfoHeadPtr->biYPelsPerMeter = 0;
			bitMapInfoHeadPtr->biClrUsed = 0; // numberEntries	
			bitMapInfoHeadPtr->biClrImportant = 0;

			CheckAndUnlockHandle (imageOverlayInfoPtr->offScreenMapHandle);

			} // end "if (resultCode == noErr)" 

		if (resultCode != noErr)
			{
			imageOverlayInfoPtr->offScreenMapHandle =
							UnlockAndDispose (imageOverlayInfoPtr->offScreenMapHandle);

			imageOverlayInfoPtr->offscreenStorageHandle =
							UnlockAndDispose (imageOverlayInfoPtr->offscreenStorageHandle);

			imageOverlayInfoPtr->offscreenMapSize = 0;

			} // end "if (resultCode != noErr)"
	#endif	// defined multispec_lin 
	
	imageOverlayInfoPtr->rowBytes = pixRowBytes;

	return (resultCode);
	
}	// end "GetOverlayOffscreenGWorld" 



//------------------------------------------------------------------------------------
//								 Copyright (1988-2018)
//								(c) Purdue Research Foundation
//									All rights reserved.
//
//	Function name:		void HideAllImageOverlays
//
//	Software purpose:	The purpose of this routine is to hide all of the image overlays.
//
//	Parameters in:				
//
//	Parameters out:	
//
//	Value Returned:	 
//
// Called By:			
//
//	Coded By:			Larry L. Biehl			Date: 01/18/2003
//	Revised By:			Larry L. Biehl			Date: 01/18/2003

Boolean HideAllImageOverlays (
				ImageOverlaySpecsPtr				imageOverlayListPtr,
				UInt32								numberImageOverlays)
				
{ 
	UInt32								index;
	
	Boolean								callFlag = FALSE;
	
					
	for (index=0; index<numberImageOverlays; index++)
		{
		if (imageOverlayListPtr[index].index > 0)
			{
			imageOverlayListPtr[index].index = -abs (imageOverlayListPtr[index].index);
			callFlag = TRUE;
			
			}	// end "if (imageOverlayListPtr[index].index > 0)"
			
		}	// end "for (index=0; index<numberImageOverlays; index++)"
		
	return (callFlag);
	
}	// end "HideAllImageOverlays"	



//------------------------------------------------------------------------------------
//								 Copyright (1988-2018)
//								(c) Purdue Research Foundation
//									All rights reserved.
//
//	Function name:		void InitializeImageOverlayInfoStructure
//
//	Software purpose:	The purpose of this routine is to initialize memory for
//							the image overlay structure.
//
//	Parameters in:		None
//
//	Parameters out:	None
//
// Value Returned:	None
// 
// Called By:			
//
//	Coded By:			Larry L. Biehl			Date: 01/03/2003
//	Revised By:			Larry L. Biehl			Date: 05/19/2017

void InitializeImageOverlayInfoStructure (
				ImageOverlayInfoPtr				imageOverlayInfoPtr)

{
	if (imageOverlayInfoPtr != NULL)
		{
		#if defined multispec_mac
			imageOverlayInfoPtr->cgInfo.contextRef = NULL;
			imageOverlayInfoPtr->cgInfo.imageRef = NULL;
			imageOverlayInfoPtr->cgInfo.contextRowBytes = 0;
		#endif	// defined multispec_mac

		#if defined multispec_win
			imageOverlayInfoPtr->overlayDC = NULL;
		#endif	// defined multispec_win
		
		imageOverlayInfoPtr->paletteObject = NULL;
		
		InitializePlanarCoordinateInfo (&imageOverlayInfoPtr->planarCoordinate);
		
		imageOverlayInfoPtr->boundingMapRectangle.top = DBL_MAX;
		imageOverlayInfoPtr->boundingMapRectangle.bottom = -DBL_MAX;
		imageOverlayInfoPtr->boundingMapRectangle.left = DBL_MAX;
		imageOverlayInfoPtr->boundingMapRectangle.right = -DBL_MAX;
		
		imageOverlayInfoPtr->lineColumnRect.top = 0;
		imageOverlayInfoPtr->lineColumnRect.left = 0;
		imageOverlayInfoPtr->lineColumnRect.bottom = 0;
		imageOverlayInfoPtr->lineColumnRect.right = 0;
		
		imageOverlayInfoPtr->offscreenGWorld = NULL;
		imageOverlayInfoPtr->offScreenMapHandle = NULL;
		imageOverlayInfoPtr->offscreenStorageHandle = NULL;
		imageOverlayInfoPtr->offscreenMapSize = 0;
		imageOverlayInfoPtr->rowBytes = 0;
		
		imageOverlayInfoPtr->usePlanarCoordinateInfoFlag = FALSE;
		
		}	// end "if (imageOverlaySpecsPtr != NULL)"
			
}	// end "InitializeImageOverlayInfoStructure"



//-----------------------------------------------------------------------------
//								 Copyright (1988-2018)
//								(c) Purdue Research Foundation
//									All rights reserved.
//
//	Function name:		void ReleaseImageOverlayStructureMemory
//
//	Software purpose:	The purpose of this routine is to dispose of the offscreen
//							memory for the window.
//
//	Parameters in:					
//
//	Parameters out:				
//
//	Value Returned:	
// 
// Called By:						
//
//	Coded By:			Larry L. Biehl			Date: 01/03/2003
//	Revised By:			Larry L. Biehl			Date: 05/19/2017	

void ReleaseImageOverlayStructureMemory (
				Handle*								imageOverlayHandlePtr,
				SInt16								overlayIndex)

{  
	ImageOverlayInfoPtr				imageOverlayInfoPtr;
				
	
	imageOverlayInfoPtr = (ImageOverlayInfoPtr)GetHandlePointer (
														imageOverlayHandlePtr[overlayIndex],
														kLock);
	
	if (imageOverlayInfoPtr != NULL)
		{															
		#if defined multispec_mac 
			GWorldPtr							offscreenGWorld;
			PixMapHandle						pixMapHandle;
		
			if (imageOverlayInfoPtr->offscreenGWorld != NULL)
				{
				#if defined multispec_win 
					if (imageOverlayInfoPtr->paletteObject != NULL)
						{ 
						delete imageOverlayInfoPtr->paletteObject;
						imageOverlayInfoPtr->paletteObject = NULL;
						
						}	// end "if (imageOverlayInfoPtr->paletteObject != NULL)" 
				#endif	// defined multispec_win 
			
				offscreenGWorld = imageOverlayInfoPtr->offscreenGWorld;
				pixMapHandle = GetPortPixMap (offscreenGWorld);
				
				UnlockPixels (pixMapHandle);
				AllowPurgePixels (pixMapHandle);
				DisposeGWorld (offscreenGWorld);
									
				imageOverlayInfoPtr->offscreenGWorld = NULL;
				imageOverlayInfoPtr->offScreenMapHandle = NULL;
				imageOverlayInfoPtr->offscreenMapSize = 0;
				
				}	// end "if (windowInfoPtr->offscreenGWorld != NULL)" 
				
			if (gOSXCoreGraphicsFlag)
				{
						// Release the core graphics image.
				
				if (imageOverlayInfoPtr->cgInfo.imageRef != NULL)		
					gCGImageReleasePtr (imageOverlayInfoPtr->cgInfo.imageRef);
					
				UnlockAndDispose (imageOverlayInfoPtr->offscreenStorageHandle);
				
				if (imageOverlayInfoPtr->offScreenMapHandle != NULL)	
					DisposePixMap ((PixMapHandle)imageOverlayInfoPtr->offScreenMapHandle);
					
				imageOverlayInfoPtr->cgInfo.imageRef = NULL;
				imageOverlayInfoPtr->cgInfo.contextRowBytes = 0;
				
				imageOverlayInfoPtr->offscreenStorageHandle = NULL;
				imageOverlayInfoPtr->offScreenMapHandle = NULL;
				imageOverlayInfoPtr->offscreenMapSize = 0;
			
				}	// end "if (gOSXCoreGraphicsFlag)"
		#endif // defined multispec_mac
				                               
		#if defined multispec_lin
				imageOverlayInfoPtr->offScreenMapHandle =
								UnlockAndDispose (imageOverlayInfoPtr->offScreenMapHandle);

				imageOverlayInfoPtr->offscreenStorageHandle =
								UnlockAndDispose (imageOverlayInfoPtr->offscreenStorageHandle);

				imageOverlayInfoPtr->offscreenMapSize = 0;
		#endif // defined multispec_lin
				                               
		#if defined multispec_win
			ReleaseImageOverlayStructureMemory (imageOverlayInfoPtr);
			/*
			imageOverlayInfoPtr->offScreenMapHandle =  
								UnlockAndDispose (imageOverlayInfoPtr->offScreenMapHandle);
			
			imageOverlayInfoPtr->offscreenStorageHandle = 							
								UnlockAndDispose (imageOverlayInfoPtr->offscreenStorageHandle);
			
			imageOverlayInfoPtr->offscreenMapSize = 0;
			*/
		#endif // defined multispec_win
		
		UnlockAndDispose (imageOverlayHandlePtr[overlayIndex]);
		imageOverlayHandlePtr[overlayIndex] = NULL;
		
		}	// end "if (imageOverlayInfoPtr != NULL)"
	
}	// end "ReleaseImageOverlayStructureMemory" 



//-----------------------------------------------------------------------------
//								 Copyright (1988-2018)
//								(c) Purdue Research Foundation
//									All rights reserved.
//
//	Function name:		void ReleaseImageOverlayStructureMemory
//
//	Software purpose:	The purpose of this routine is to dispose of the offscreen
//							memory for the specified overlay.
//
//	Parameters in:					
//
//	Parameters out:				
//
//	Value Returned:	
// 
// Called By:						
//
//	Coded By:			Larry L. Biehl			Date: 05/19/2017
//	Revised By:			Larry L. Biehl			Date: 05/19/2017	

void ReleaseImageOverlayStructureMemory (
				ImageOverlayInfoPtr				imageOverlayInfoPtr)

{
	#if defined multispec_win
		if (imageOverlayInfoPtr->offScreenMapHandle != NULL)
			DeleteObject (imageOverlayInfoPtr->offScreenMapHandle);

		if (imageOverlayInfoPtr->overlayDC != NULL)
			DeleteDC (imageOverlayInfoPtr->overlayDC);

		imageOverlayInfoPtr->offScreenMapHandle = NULL;
		imageOverlayInfoPtr->offscreenStorageHandle = NULL;
		imageOverlayInfoPtr->overlayDC = NULL;
		imageOverlayInfoPtr->offscreenMapSize = 0;
	#endif	// defined multispec_win

}	// end "ReleaseImageOverlayStructureMemory" 



//------------------------------------------------------------------------------------
//								 Copyright (1988-2018)
//								(c) Purdue Research Foundation
//									All rights reserved.
//
//	Function name:		void SetUpImageOverlayInformation
//
//	Software purpose:	This routine unlocks the requested offscreen buffer handle.
//
//	Parameters in:				
//
//	Parameters out:				
//
//	Value Returned:	
// 
// Called By:			ClassifyControl in SClassify.cpp
//							ClusterControl in SCluster.cpp
//							
//
//	Coded By:			Larry L. Biehl			Date: 01/14/2003
//	Revised By:			Larry L. Biehl			Date: 03/30/2007			

SInt16 SetUpImageOverlayInformation (
				Handle								imageWindowInfoHandle,
				SInt16								overlayIndex,
				UInt32								numberClasses,
				LongRect*							overlayBoundingAreaPtr,
				SInt16								thematicPaletteType,
				SInt16								defaultTransparency)

{  
	ImageOverlayInfoPtr				imageOverlayInfoPtr;
	WindowInfoPtr						imageWindowInfoPtr;
	
	Handle								imageFileInfoHandle,
											imageMapProjectionHandle,
											imageOverlayInfoHandle = NULL;
											
	SInt32								numberColumns,
											numberLines;
	
	SInt16								overlayListIndex,
											returnCode;
	
	Boolean								continueFlag = FALSE,
											newOverlayFlag;
	
	
			// Now get the image overlay memory and the index to that memory in the
			// image overlay structure list.	
																
	if (imageWindowInfoHandle != NULL)
		{	
		continueFlag = TRUE;
				
		newOverlayFlag = FALSE;
		if (overlayIndex == -1)
			newOverlayFlag = TRUE;
			
				// Verify that there is room in the window overlay list for any new
				// overlay.
				
		imageWindowInfoPtr = (WindowInfoPtr)GetHandlePointer (imageWindowInfoHandle);
													
		if (newOverlayFlag)
			overlayListIndex = -1;
			
		else	// !newOverlayFag
			{								
			overlayListIndex = GetWindowImageOverlayIndex (imageWindowInfoPtr,
																			overlayIndex);
																			
			}	// end "else !newOverlayFag"
			
		if (overlayListIndex == -1 && 
						imageWindowInfoPtr->numberImageOverlays >= kMaxNumberImageOverlays)
			continueFlag = FALSE;
			
		}	// end "if (imageWindowInfoHandle != NULL)"
			
	if (continueFlag)
		{					
		imageOverlayInfoHandle = GetImageOverlayInfoMemory (&overlayIndex);
		
				// Get pointer the to overlay info structure. The handle is already
				// locked.
					
		imageOverlayInfoPtr = (ImageOverlayInfoPtr)GetHandlePointer (
																			imageOverlayInfoHandle);
		
		continueFlag = (imageOverlayInfoPtr != NULL);
			
		}	// end "if (continueFlag)"
	
	if (continueFlag)
		{								
		returnCode = GetOverlayOffscreenGWorld (numberClasses,
																overlayBoundingAreaPtr->left,
																overlayBoundingAreaPtr->right,
																overlayBoundingAreaPtr->top,
																overlayBoundingAreaPtr->bottom,
																thematicPaletteType,
																imageOverlayInfoPtr);
			
		if (returnCode == noErr)
			{
			imageWindowInfoPtr = (WindowInfoPtr)GetHandlePointer (imageWindowInfoHandle);	
												
			overlayListIndex = GetWindowImageOverlayIndex (imageWindowInfoPtr,
																			overlayIndex);
									
			if (overlayListIndex == -1)
				{
				overlayListIndex = (SInt16)imageWindowInfoPtr->numberImageOverlays;
				imageWindowInfoPtr->numberImageOverlays++;
				
				}	// end "if (overlayListIndex == -1)"
			
			imageWindowInfoPtr->imageOverlayList[overlayListIndex].index = 
																					-(overlayIndex+1);
												
			imageWindowInfoPtr->imageOverlayList[overlayListIndex].opacity = 
															(float)(100-defaultTransparency)/100;
															
					// Hide all current overlays for the image window.
					
			HideAllImageOverlays (imageWindowInfoPtr->imageOverlayList, 
											imageWindowInfoPtr->numberImageOverlays);
			
			GetDefaultImageOverlayName (overlayIndex);
			
			if (FindIfMapInformationExists (imageWindowInfoPtr))
				{
				imageFileInfoHandle = GetFileInfoHandle (imageWindowInfoHandle);
				imageMapProjectionHandle = GetFileMapProjectionHandle (imageFileInfoHandle);
				
				if (GetPolynomialOrder (imageMapProjectionHandle) <= 0)
					{
					numberColumns = overlayBoundingAreaPtr->right - 
																		overlayBoundingAreaPtr->left + 1;
					numberLines = overlayBoundingAreaPtr->bottom - 
																		overlayBoundingAreaPtr->top + 1;
					
					UpdatePlanarCoordinateInfo (imageMapProjectionHandle,
															&imageOverlayInfoPtr->planarCoordinate,
															overlayBoundingAreaPtr->left,
															overlayBoundingAreaPtr->top, 
															numberColumns,
															numberLines,
															1,
															1,
															FALSE,
															FALSE);

					SetBoundingMapRectangle (&imageOverlayInfoPtr->planarCoordinate,
														numberColumns,
														numberLines,
														&imageOverlayInfoPtr->boundingMapRectangle,
														kUpperLeftLowerRightCorners);
					
					imageOverlayInfoPtr->usePlanarCoordinateInfoFlag = TRUE;
						
					}	// end "if (GetPolynomialOrder (mapProjectionHandle) <= 0)"
				
				else	// GetPolynomialOrder (imageMapProjectionHandle) > 0
					imageOverlayInfoPtr->usePlanarCoordinateInfoFlag = FALSE;
										
				}	// end "if (FindIfMapInformationExists (imageWindowInfoPtr))"
				
			else	// !FindIfMapInformationExists (imageWindowInfoPtr)
				imageOverlayInfoPtr->usePlanarCoordinateInfoFlag = FALSE;
				
			imageOverlayInfoPtr->lineColumnRect = *overlayBoundingAreaPtr;
			
			}	// end "if (returnCode == noErr)"
										
		else	// returnCode != noErr
			overlayIndex = -1;
			
		CheckAndUnlockHandle (imageOverlayInfoHandle);
			
		}	// end "if (continueFlag)"
		
	else	// !continueFlag
		{
				// Not possible to create an image overlay.
				
		overlayIndex = -1;
		
		}	// end "else !continueFlag"
		
	if (overlayIndex == -1)
		DisplayAlert (kErrorAlertID,
							kCautionAlert, 
							kAlertStrID, 
							IDS_Alert134,
							0, 
							NULL);	
		
	return (overlayIndex);
	
}	// end "SetUpImageOverlayInformation" 



//------------------------------------------------------------------------------------
//								 Copyright (1988-2018)
//								(c) Purdue Research Foundation
//									All rights reserved.
//
//	Function name:		void UnlockImageOverlayInfoHandle
//
//	Software purpose:	This routine unlocks or keeps locked the requested
//							image overlay information handle depending on the input handle
//							status byte.
//
//	Parameters in:				
//
//	Parameters out:				
//
//	Value Returned:	
// 
// Called By:
//
//	Coded By:			Larry L. Biehl			Date: 01/20/2003
//	Revised By:			Larry L. Biehl			Date: 01/20/2003			

void UnlockImageOverlayInfoHandle (
				SInt16								overlayIndex,
				SignedByte							handleStatus)

{  
	Handle*								imageOverlayHandlePtr;
	ImageOverlayInfoPtr				imageOverlayInfoPtr;
	

	imageOverlayInfoPtr = NULL;
	if (overlayIndex >= 0 && (UInt32)overlayIndex < gImageOverlayHandleListLength)
		{
		imageOverlayHandlePtr = (Handle*)GetHandlePointer (gImageOverlayHandle);
	
		if (imageOverlayHandlePtr != NULL)										
			MHSetState (imageOverlayHandlePtr[overlayIndex], handleStatus);
													
		}	// end "if (overlayIndex >= 0 && ..."
		
}	// end "UnlockImageOverlayInfoHandle"



//------------------------------------------------------------------------------------
//								 Copyright (1988-2018)
//								(c) Purdue Research Foundation
//									All rights reserved.
//
//	Function name:		void UnlockImageOverlayOffscreenBuffer
//
//	Software purpose:	This routine unlocks the requested offscreen buffer handle.
//
//	Parameters in:				
//
//	Parameters out:				
//
//	Value Returned:	
// 
// Called By:
//
//	Coded By:			Larry L. Biehl			Date: 01/11/2003
//	Revised By:			Larry L. Biehl			Date: 05/19/2017			

void UnlockImageOverlayOffscreenBuffer (
				ImageOverlayInfoPtr				imageOverlayInfoPtr)

{  
	if (imageOverlayInfoPtr != NULL)
		{
		#if defined multispec_mac 
			if (gOSXCoreGraphicsFlag)
				{
						// This handle needs to stay locked.
				//CheckAndUnlockHandle (imageOverlayInfoPtr->offscreenStorageHandle);
				
				}	// end "if (gOSXCoreGraphicsFlag)"
				
			else	// !gOSXCoreGraphicsFlag
				{
				
				}	// end "else !gOSXCoreGraphicsFlag"
		#endif // defined multispec_mac 
		
		#if defined multispec_lin
			CheckAndUnlockHandle (imageOverlayInfoPtr->offscreenStorageHandle);
		#endif	// defined multispec_lin
		
		#if defined multispec_win
					// Do not unlock this handle which is now a HBITMAP
			//CheckAndUnlockHandle (imageOverlayInfoPtr->offscreenStorageHandle);
		#endif // defined multispec_win
		
		}	// end "if (overlayIndex >= 0 && ..."
	
}	// end "UnlockImageOverlayOffscreenBuffer"

