//	 									MultiSpec
//
//					Laboratory for Applications of Remote Sensing
//									Purdue University
//								West Lafayette, IN 47907
//								 Copyright (1988-2018)
//							(c) Purdue Research Foundation
//									All rights reserved.
//
//	File:						MSelectionArea.c
//
//	Authors:					Larry L. Biehl
//
//	Revision date:			12/07/2017
//
//	Language:				C
//
//	System:					Macintosh Operating System
//
//	Brief description:	This file contains functions that handle area selections
//								in image windows.
//
//	Functions in file:	void 					PolygonSelection
//								void 					RectangleSelection
//								void 					SelectArea
//								void 					ShowSelection
//
//	Diagram of MultiSpec routine calls for the routines in the file.
//
//		SelectArea
//			RectangleSelection
//			LoadRectStatNewFieldW (in statistics.c)
//			LoadPolyStatNewFieldW (in statistics.c)
//
//
//	Include files:			"MultiSpecHeaders"
//
//------------------------------------------------------------------------------------

//#include	"SMultiSpec.h"
//#include "SExtGlob.h" 
#include	"MGraphView.h"

extern void ComputeSelectionLineColumns (
				DisplaySpecsPtr					displaySpecsPtr,
				MapProjectionInfoPtr				mapProjectionInfoPtr,
				LongRect*							lineColumnRectPtr,
				Rect*									displayRectPtr,
				LongPoint*							startPointPtr,
				DoubleRect*							coordinateRectPtr);

								

		// Prototypes for routines in this file that are only called by
		// other routines in this file.				

Boolean 	CheckIfCanScrollImage (
				Point									newPt,
				Rect*									viewRectPtr,
				SInt32								hOffset,
				SInt32								vOffset);			

void 		PolygonSelection (
				Point									startPt,
				SelectionInfoPtr					selectionInfoPtr, 
				DisplaySpecsPtr					displaySpecsPtr);

void 		RectangleSelection (
				Point									startPt,
				SelectionInfoPtr					selectionInfoPtr,
				DisplaySpecsPtr					displaySpecsPtr);



//------------------------------------------------------------------------------------
//								 Copyright (1988-2018)
//								(c) Purdue Research Foundation
//									All rights reserved.
//
//	Function name:		Boolean CheckIfCanScrollImage
//
//	Software purpose:	This routine determines whether the image should be scrolled. It
//							takes into account the last time that the window was scrolled to
//							be sure that the window does not scroll too quickly unless the
//							user has placed the mouse point more than a scrollbar width away
//							from the window.
//
//	Parameters in:				
//
//	Parameters out:				
//
// Value Returned:	True, if the window should be scrolled
//							False if the window should not be scrolled.
// 
// Called By:
//
//	Coded By:			Larry L. Biehl			Date: 02/17/2000
//	Revised By:			Larry L. Biehl			Date: 02/17/2000			

Boolean CheckIfCanScrollImage  (
				Point									newPt,
				Rect*									viewRectPtr,
				SInt32								hOffset,
				SInt32								vOffset)
				
{	
	SInt32								hControlMaximum,
											vControlMaximum;
											
	Boolean								scrollFlag = FALSE;
	
	
	GetHorAndVerControlMaximums (gActiveImageWindow,
											&hControlMaximum,
											&vControlMaximum);
	
	if ((newPt.h < viewRectPtr->left && hOffset > 0) ||
			(newPt.h > viewRectPtr->right && hOffset < hControlMaximum) || 
				(newPt.v < viewRectPtr->top && vOffset > 0) ||
					(newPt.v > viewRectPtr->bottom && vOffset < vControlMaximum))
		{
		scrollFlag = TRUE;  
	
				// Make sure that we do not scroll too fast.
											
		if (TickCount () < gNextTime)
			{
					// If mouse point is more than scrollbar width away from
					// window, then ignore the time check.
			
			if (newPt.h > viewRectPtr->left - kSBarWidth &&
					newPt.h < viewRectPtr->right + kSBarWidth && 
		  			newPt.v > viewRectPtr->top - kSBarWidth &&
					newPt.v < viewRectPtr->bottom + kSBarWidth)
				scrollFlag = FALSE;  
			
			}	// end "if (TickCount () < gNextTime)"
		
		}	// end "if ((newPt.h < viewRectPtr->left && hOffset > 0) || ..."
		
	if (scrollFlag)
		gNextTime = TickCount () + 4;
														
	return (scrollFlag);					

}	// end "CheckIfCanScrollImage"



//------------------------------------------------------------------------------------
//								 Copyright (1988-2018)
//								(c) Purdue Research Foundation
//									All rights reserved.
//
//	Function name:		void PolygonSelection
//
//	Software purpose:	This routine handles getting a vertex that is a
//							part of a polyon that the user is selecting to
//							define an area. This routine will save the selected
//							point in the list of selection polygon points for the
// 						active image window, so that it can be used for any
//							future updates.
//
//	Parameters in:				
//
//	Parameters out:				
//
// Value Returned:	None
// 
// Called By:
//
//	Coded By:			Larry L. Biehl			Date: 08/30/1989
//	Revised By:			Larry L. Biehl			Date: 06/19/2002			

void PolygonSelection  (
				Point									startPt, 		//	Start point for selection
																			// rectangle.
				SelectionInfoPtr					selectionInfoPtr, 
				DisplaySpecsPtr					displaySpecsPtr)
				
{	
	LCToWindowUnitsVariables		lcToWindowUnitsVariables;
	
	LongPoint							lastLineColPoint,
											longPoint,
											newLineColPoint,
											newWindowPoint,
											windowPoint;
											
	Pattern								grayPattern;
	
	PenState								permanentLinePenState,
											temporaryLinePenState;
	
	Point									lastPt,
											//newOffscreenPoint,
			 								newPt,
			 								originalLastPt;
			 								
	Rect									coordinateAreaRect,
											viewRect;
	
	HPFieldPointsPtr					selectionPointsPtr;
	
	UInt32								bytesNeeded,
											bytesNeededIncrement,
											memoryLimitNumber;
	
	SInt32								doubleClickTime,
											downClickTime;
	
	SInt16								hOffset,
											index,
											startChannel,
											theChar,
											vOffset;
	
	Boolean								doubleClickEvent,
											drawNewTempLineFlag,
											drawTempLineFlag,
											scrollFlag;
											
	
			// Exit if this is a mouse down which activates this window.
			// We will assume that the user was just activating the image not
			// making a selection.										
					
	if (gNewSelectedWindowFlag && !StillDown ())
																									return;
			
	if (!InitializePolygonSelection (selectionInfoPtr,
												&selectionPointsPtr,
												&memoryLimitNumber,
												&bytesNeeded,
												&bytesNeededIncrement))
																									return;
																							
			//	Get the window limit rectangle for the selection polygon.		

	GetWindowClipRectangle (gActiveImageWindow, kImageFrameArea, &viewRect);
	ClipRect (&viewRect);
	
			// Get the coordinate area of the window.
			
	GetWindowClipRectangle (gActiveImageWindow, kCoordinateArea, &coordinateAreaRect);
		
	longPoint.h = startPt.h;
	longPoint.v = startPt.v;		
	startChannel = GetSelectionRectangleLimits (TRUE, 
																0,
																&longPoint, 
																displaySpecsPtr,
																&viewRect,
																&gTempRect,
																&gTempLongPoint);
														
			// Set the global variables needed to convert from line-column		
			// units to window units.														
				
	SetChannelWindowVariables (kToImageWindow, gActiveImageWindow, kNotCoreGraphics);
				
	SetLCToWindowUnitVariables (gActiveImageWindowInfoH,
											kToImageWindow, 
											FALSE,
											&lcToWindowUnitsVariables);
	
	SetPortWindowPort (gActiveImageWindow);
	
			// Get the pen state for drawing permanent sections of polygon.		
			
	PenPat ((ConstPatternParam)&gOutlinePenPattern);
	PenMode (patCopy);
	PenSize (1,1);
	GetPenState (&permanentLinePenState);
		
	PenPat (GetQDGlobalsGray (&grayPattern));
	PenMode (notPatXor);
	PenSize (1,1);
	GetPenState (&temporaryLinePenState);
	
	doubleClickTime = GetDblTime ();
	doubleClickEvent = FALSE;
	
	originalLastPt = startPt;
	
			// Force start point to be within the image.									
			
	newPt.h = MIN (startPt.h, gTempRect.right);
	newPt.v = MIN (startPt.v, gTempRect.bottom);
	
	do
		{
		if (selectionInfoPtr->numberPoints == 0)
			{		
					// Initialize those variables for case when there are no			
					// selected coordinates for the list.									
					
			lastLineColPoint.h = -1;
			lastLineColPoint.v = -1;
			
			lastPt = startPt = newPt;
			
			}	// end "if (selectionInfoPtr->numberPoints == 0)" 
	
		downClickTime = gEventRecord.when + doubleClickTime;
	
				// Determine if user did a double click.									
				
		while (gEventRecord.when < downClickTime)
			{
			doubleClickEvent = GetNextEvent (mDownMask, &gEventRecord);
			GlobalToLocal (&gEventRecord.where);
			
			if ((abs (gEventRecord.where.h-originalLastPt.h) +
												abs (gEventRecord.where.v-originalLastPt.v)) > 6)
				{
				downClickTime = 0;
				doubleClickEvent = FALSE;
				
				}	// end "if ((abs (gEventRecord.where.h-..."
			
			if (doubleClickEvent)
				downClickTime = 0;
			
			}	// end "while (gEventRecord.when - ..." 
			
				// Enter the point into the list if it is not the same as the last point.
				// Convert window point to line-column point								
		
		longPoint.h = lastPt.h;
		longPoint.v = lastPt.v;		
		ConvertWinPointToLC (&longPoint, &newLineColPoint);
		
		if ((lastLineColPoint.h != newLineColPoint.h) || 
														(lastLineColPoint.v != newLineColPoint.v))
			{		
			if (selectionInfoPtr->numberPoints == memoryLimitNumber)
				{
						// Change size of handle for selection polygon coordinates.	
				
				HUnlock (selectionInfoPtr->polygonCoordinatesHandle);
				bytesNeeded += bytesNeededIncrement;
				SetHandleSize (selectionInfoPtr->polygonCoordinatesHandle, bytesNeeded);
				HLock (selectionInfoPtr->polygonCoordinatesHandle);
				selectionPointsPtr = 
							(HPFieldPointsPtr)*selectionInfoPtr->polygonCoordinatesHandle;
				
						// Check if memory is full. 											
						
				if (MemError () != noErr)
					{
					PenNormal ();
																									return;
																							
					}	// end "if (MemError () != noErr)" 
					
				memoryLimitNumber += 100;
					
				}	// end "if (...->numberPoints == memoryLimitNumber)" 
				
					// Now save the line column values for the point.					
							
			index = selectionInfoPtr->numberPoints;										
			selectionPointsPtr[index].line = newLineColPoint.v;
			selectionPointsPtr[index].col = newLineColPoint.h;
			
					// Now get the window point that represents the upper left corner
					// of the pixel in case the image is zoomed in. Also allow for a
					// side-by-side display.
				
			ConvertLCToWinPoint (&newLineColPoint, 
										&newWindowPoint,
										&lcToWindowUnitsVariables);
											
			newWindowPoint.h +=
							startChannel * gChannelWindowInterval - gChannelWindowOffset;
			
			if (selectionInfoPtr->numberPoints > 0)
				{	
						// Erase the temporary line.											
									
				MoveTo (startPt.h, startPt.v);
				LineTo (newPt.h, newPt.v);
			
						// Draw the permanent line.												
					
				SetPenState (&permanentLinePenState);
				
				MoveTo (startPt.h, startPt.v);
							
				LineTo ((SInt16)newWindowPoint.h, (SInt16)newWindowPoint.v);
				
				}	// end "if (selectionInfoPtr->numberPoints > 0)" 
				
					// Set the last point to this new window point so that the
					// temporary line will be draw from the last selected point
					// which will be at the upper left position of the pixel. The
					// current last point may be somewhere else within the pixel.
					
			lastPt.h = (SInt16)newWindowPoint.h;
			lastPt.v = (SInt16)newWindowPoint.v;
								
					// Now increment the number of points for the polygon.			
					
			selectionInfoPtr->numberPoints++;
			
			AddPolyPointStatNewFieldW (newLineColPoint);
			
			lastLineColPoint = newLineColPoint;
			
			}	// end "if (lastLineColPoint.h != newLineColPoint.h && ..."
			
		else	// lastLineColPoint == newLineColPoint
			lastPt = startPt;
		
		if (!doubleClickEvent)
			{
					// Set the new start point.												
					
			startPt = lastPt;
			SetPenState (&temporaryLinePenState);
			drawTempLineFlag = TRUE;
		
			while (drawTempLineFlag)
				{
				drawTempLineFlag = !GetNextEvent (mDownMask+keyDownMask, &gEventRecord);
				
				if (!drawTempLineFlag)
					{
					if (gEventRecord.what == keyDown)
						{
						theChar = gEventRecord.message&charCodeMask;
						if (theChar == 0x08 && selectionInfoPtr->numberPoints > 0)
							{
									// Delete the last selected polygon point.			
				
							selectionInfoPtr->numberPoints--;
							DeletePolyPointStatNewFieldW (selectionInfoPtr->numberPoints);
							
									// Erase the temporary line.								
												
							MoveTo (startPt.h, startPt.v);
							LineTo (lastPt.h, lastPt.v);
											
									// Draw the new temporary line.							
									
							if (selectionInfoPtr->numberPoints > 0)
								{
										// Now update the section of the permanent ine.
										
								originalLastPt = startPt;
							
								//index = 2 * selectionInfoPtr->numberPoints - 1;
								index = selectionInfoPtr->numberPoints - 1;
								lastLineColPoint.v = selectionPointsPtr[index].line;
								lastLineColPoint.h = selectionPointsPtr[index].col;
													
								ConvertLCToWinPoint (&lastLineColPoint, 
															&windowPoint,
															&lcToWindowUnitsVariables);
												
										// Adjust the window units to reflect the channel that
										// the selection was made within.

								startPt.h = (SInt16)(windowPoint.h - gChannelWindowOffset + 
															startChannel * gChannelWindowInterval);
								startPt.v = (SInt16)windowPoint.v;
								
								Pt2Rect (originalLastPt, startPt, &gGrayRect);
								gGrayRect.bottom++;
								gGrayRect.right++;
								CopyOffScreenImage (gActiveImageWindow, 
															NIL, 
															&gGrayRect, 
															kDestinationCopy, 
															kNotFromImageUpdate);
								ClipRect (&viewRect);
								
										// Now draw the new temporary line.					
										
								MoveTo (startPt.h, startPt.v);
								LineTo (lastPt.h, lastPt.v);
								
								}	// end "if (...->numberPoints > 0)" 
											
							}	// end "if (theChar == 0x08 && ...)" 
						
						drawTempLineFlag = TRUE;
						
						}	// end "if (gEventRecord.what == keyDown)" 
						
					}	// end "if (!drawTempLineFlag)" 
					
				newPt = gEventRecord.where;
				GlobalToLocal (&newPt);
						
				drawNewTempLineFlag = FALSE;
					
				if (drawTempLineFlag)
					{
					if (selectionInfoPtr->numberPoints == 0)
						lastPt = newPt;
					
					if (*(SInt32*)&newPt != *(SInt32*)&lastPt)
						{	
								// Now determine if point is outside of window available 
								// for image. If so, then the image may need to be scrolled.
						
						if (!PtInRect (newPt, &viewRect))
							{
									// The new point is not within the visible part of the 
									// image, try scrolling the imaging to make the point 
									// visible.
							
							hOffset = displaySpecsPtr->origin[kHorizontal];
							vOffset = displaySpecsPtr->origin[kVertical];	
							
									// Check if scrolling is possible.
							
							scrollFlag = CheckIfCanScrollImage  (newPt,
																				&viewRect,
																				hOffset,
																				vOffset);
																				
							if (scrollFlag)
								{			
										// Erase old temporary line.									
										
								MoveTo (startPt.h, startPt.v);
								LineTo (lastPt.h, lastPt.v);
								
								drawNewTempLineFlag = TRUE;
							
								ImageWindowSelectionScroll (newPt, &viewRect);
							
								vOffset -= displaySpecsPtr->origin[kVertical];
								hOffset -= displaySpecsPtr->origin[kHorizontal];
							
								#ifdef powerc
									vOffset = vOffset * displaySpecsPtr->magnification;
									hOffset = hOffset * displaySpecsPtr->magnification;
								#else	// !powerc 
									vOffset *= displaySpecsPtr->magnification;
									hOffset *= displaySpecsPtr->magnification;
								#endif
								
								startPt.v += vOffset;
								startPt.h += hOffset;
						
								if (hOffset != 0 ||  vOffset != 0)
									{
									longPoint.h = startPt.h;
									longPoint.v = startPt.v;
									startChannel = GetSelectionRectangleLimits (
																						FALSE,
																						startChannel, 
																						&longPoint, 
																						displaySpecsPtr,
																						&viewRect,
																						&gTempRect,
																						&gTempLongPoint);
									
									}	// end "if (hOffset != 0 ||  vOffset != 0)"
				
										// Update parameters to convert from line/columns to
										// window units.
										
								SetLCToWindowUnitVariables (gActiveImageWindowInfoH,
																		kToImageWindow, 
																		FALSE,
																		&lcToWindowUnitsVariables);
												
								}	// end "if (scrollFlag)"
								
							}	// end "if (!PtInRect (newPt, &viewRect))" 
							
						}	// end "if (*(SInt32*)&newPt != ..."
						
					}	// end "if (drawTempLineFlag)" 
							
				originalLastPt = newPt;
					
				newPt.v = MIN (newPt.v, gTempRect.bottom-1);
				newPt.h = MIN (newPt.h, gTempRect.right-1);
					
				newPt.v = MAX (newPt.v, gTempRect.top);
				newPt.h = MAX (newPt.h, gTempRect.left);
			
				if (newPt.h != lastPt.h || newPt.v != lastPt.v)
					{
							// Erase current temporary line if needed.
					
					if (!drawNewTempLineFlag)
						{
						MoveTo (startPt.h, startPt.v);
						LineTo (lastPt.h, lastPt.v);
						
						}	// end "if (!drawNewTempLineFlag)"
						
					drawNewTempLineFlag = TRUE;
				
					lastPt = newPt;
					
					}	// end "if (newPt.h != lastPt.h || newPt.v != lastPt.v)"
							
				if (drawNewTempLineFlag)
					{
							// Draw new temporary line.										
								
					MoveTo (startPt.h, startPt.v);
					LineTo (newPt.h, newPt.v);
					
					}	// end "if (drawNewTempLineFlag)" 
	
				ClipRect (&coordinateAreaRect);
				UpdateCursorCoordinates (newPt);
				ClipRect (&viewRect);
				//DrawCursorCoordinates (gActiveImageWindowInfoH);
					
				}	// end "while (drawTempLineFlag)"
					
			}	// end "if (!doubleClickEvent)" 
			
		}		while (!doubleClickEvent);
	
	PenNormal ();
		
			// Get the mouse up event to determine whether the command key was down.
			
	CheckSomeEvents (mUpMask);
	
			// Get the selection bounding lineColumn and offscreen rectangles.		
                         
	GetBoundingSelectionRectangles (displaySpecsPtr,
												selectionInfoPtr, 
												selectionPointsPtr,
												startChannel);
	
			// Now also get the bounding area in coordinate units.
						
	ComputeSelectionCoordinates (gActiveImageWindowInfoH,
											&selectionInfoPtr->lineColumnRectangle,
											&selectionInfoPtr->coordinateRectangle);
                         
	ClosePolygonSelection (selectionInfoPtr);
	
	InvalidateWindow (gActiveImageWindow, kCoordinateSelectionArea, FALSE);
	
}	// end "PolygonSelection" 



//------------------------------------------------------------------------------------
//								 Copyright (1988-2018)
//								(c) Purdue Research Foundation
//									All rights reserved.
//
//	Function name:		RectangleSelection
//
//	Software purpose:	This routine draws the gray rectangle during a mouse down and
//							drag event in the image window.
//
//	Parameters in:					
//
//	Parameters out:				
//
// Value Returned:	None
// 
// Called By:			SelectArea
//
//	Coded By:			Ravi S. Budruk			Date: 07/25/1988
//	Revised By:			Ravi S. Budruk			Date: 08/09/1988	
//	Revised By:			Larry L. Biehl			Date: 04/29/2012			

void RectangleSelection  (
				Point									startPt, 
				SelectionInfoPtr					selectionInfoPtr,		//	Start point for
																					// selection rectangle.
				DisplaySpecsPtr					displaySpecsPtr)
				
{	
	DoubleRect							coordinateRect;
	
	//PenState							penState;
	
	LongPoint							longPoint;
	
	Point 								lastPt,
											newPt;
	
	Rect									coordinateAreaRect,
											newGrayRect,
											viewRect;
	
	Pattern								grayPattern;
											
	//PlanarCoordinateSystemInfoPtr	planarCoordinateSystemInfoPtr;
	
	//double								hConversionFactor,
	//										vConversionFactor;
	
	SInt64								numberPixels;
	
	Handle 								mapProjectionHandle;
	
	SInt32								hOffset,
											startChannel,
											vOffset;
											
	Boolean								coordinateFlag,
											scrollFlag,
											selectionClearedFlag;
											
											
			// Initialize local variables
			
	coordinateFlag = (GetCoordinateHeight (gActiveImageWindowInfoH) > 0);
	//planarCoordinateSystemInfoPtr = NULL;
				
	mapProjectionHandle = GetFileMapProjectionHandle2 (gActiveImageWindowInfoH);

	MapProjectionInfoPtr mapProjectionInfoPtr = 
					(MapProjectionInfoPtr)GetHandlePointer (mapProjectionHandle, kLock);
	
	//if (mapProjectionInfoPtr != NULL)
	//	planarCoordinateSystemInfoPtr = &mapProjectionInfoPtr->planarCoordinate;
			
			// Define initial coordinates of the gray rectangle						
			
	gGrayRect.top = startPt.v;
	gGrayRect.left = startPt.h;
	
			// Get the magnification factors to use to convert from screen			
			// elements to lines and columns													
	/*
	vConversionFactor = displaySpecsPtr->displayedLineInterval/
																displaySpecsPtr->magnification;
																
	hConversionFactor = displaySpecsPtr->displayedColumnInterval/
																displaySpecsPtr->magnification;
	*/
			//	Get the limit rectangle for the gray selection rectangle. This will be the
			// image area of the window.			

	GetWindowClipRectangle (gActiveImageWindow, kImageFrameArea, &viewRect);
	
			// Get the coordinate area of the window.
			
	GetWindowClipRectangle (gActiveImageWindow, kCoordinateArea, &coordinateAreaRect);
	
	longPoint.h = startPt.h;
	longPoint.v = startPt.v;
	startChannel = GetSelectionRectangleLimits (TRUE,
																0, 
																&longPoint, 
																displaySpecsPtr,
																&viewRect,
																&gTempRect,
																&gTempLongPoint);
	
			// Force start point to be within the image.									
			
	startPt.h = MIN (startPt.h, gTempRect.right);
	startPt.v = MIN (startPt.v, gTempRect.bottom);
																				
	SetRect (&gGrayRect, startPt.h, startPt.v, startPt.h, startPt.v);
	
	ComputeSelectionLineColumns (displaySpecsPtr, 
											mapProjectionInfoPtr,
											//hConversionFactor,
											//vConversionFactor,
											&gTempLongRect,
											&gGrayRect,
											&gTempLongPoint,
											&coordinateRect);
			
	lastPt = startPt;
	SetPortWindowPort (gActiveImageWindow);
	PenMode (patXor);
	PenPat (GetQDGlobalsGray (&grayPattern));
	
	newPt = startPt;
	while (StillDown ())
		{
		GetMouse (&newPt);
		
				// Check if mouse has moved.
		
		if (*(SInt32*)&newPt != *(SInt32*)&lastPt)
			{	
			selectionClearedFlag = false;
			
					// Now determine if point is outside of window available for image.
					// If so, then the image may need to be scrolled.
					
			if (!PtInRect (newPt, &viewRect))
				{
						// The new point is not within the visible part of the image,
						// try scrolling the imaging to make the point visible.
				
				hOffset = displaySpecsPtr->origin[kHorizontal];
				vOffset = displaySpecsPtr->origin[kVertical];	
							
						// Check if scrolling is possible.
				
				scrollFlag = CheckIfCanScrollImage  (newPt,
																	&viewRect,
																	hOffset,
																	vOffset);
				if (scrollFlag)
					{  
							// Scrolling is possible; 
							// remove the current selection before scrolling.
							
					FrameRect (&gGrayRect);
					selectionClearedFlag = true;
			
							// Scroll the image.
				
					ImageWindowSelectionScroll (newPt, &viewRect);
				
					vOffset -= displaySpecsPtr->origin[kVertical];
					hOffset -= displaySpecsPtr->origin[kHorizontal];
							
					vOffset = (SInt32)(vOffset * displaySpecsPtr->magnification);
					hOffset = (SInt32)(hOffset * displaySpecsPtr->magnification);
							
					startPt.v += vOffset;
					startPt.h += hOffset;
					
					lastPt.v += vOffset;
					lastPt.h += hOffset;
			
					if (hOffset != 0 ||  vOffset != 0)
						{
						longPoint.h = startPt.h;
						longPoint.v = startPt.v;
						startChannel = GetSelectionRectangleLimits (FALSE,
																					startChannel, 
																					&longPoint, 
																					displaySpecsPtr,
																					&viewRect,
																					&gTempRect,
																					&gTempLongPoint);
						
						}	// end "if (hOffset != 0 ||  vOffset != 0)"
									
					}	// end "if (scrollFlag)"
				
				}	// end "if (!PtInRect (newPt, &viewRect))"
				
					// Now determine if the newPt needs to be adjusted because of
					// pegging to the edge of the image.
					
			newPt.v = MIN (newPt.v, gTempRect.bottom-1);
			newPt.h = MIN (newPt.h, gTempRect.right-1);
		
			newPt.v = MAX (newPt.v, gTempRect.top);
			newPt.h = MAX (newPt.h, gTempRect.left);
			
			Pt2Rect (startPt, newPt, &newGrayRect);
			
			if (newGrayRect.top != gGrayRect.top || 
						newGrayRect.left != gGrayRect.left ||
								newGrayRect.bottom != gGrayRect.bottom || 
										newGrayRect.right != gGrayRect.right)
				{
				if (!selectionClearedFlag)
					FrameRect (&gGrayRect);
					
				selectionClearedFlag = true;
					
				gGrayRect = newGrayRect;
				
				}	// end "if (newGrayRect != gGrayRect)"
				
			if (selectionClearedFlag)
				FrameRect (&gGrayRect);
				
			lastPt = newPt;
	
			if (coordinateFlag)
				{		
				ClipRect (&coordinateAreaRect);
						
				ComputeSelectionLineColumns (displaySpecsPtr,
														mapProjectionInfoPtr,
														//hConversionFactor,
														//vConversionFactor,
														&gTempLongRect,
														&gGrayRect,
														&gTempLongPoint,
														&coordinateRect);
					
				numberPixels = (SInt64)(gTempLongRect.bottom - gTempLongRect.top + 1) * 
												(gTempLongRect.right - gTempLongRect.left + 1);
					
				UpdateCursorCoordinates (newPt);
													
				DrawSelectionCoordinates (gActiveImageWindowInfoH,
													&gTempLongRect,
													&coordinateRect,
													numberPixels);
					
				ClipRect (&viewRect);
				
				}	// end "if (coordinateFlag)" 
			
			}	// end "if ((newPt.v != lastPt.v) || ..." 
			
		}	// end "while (StillDown ())" 
		
	if (gNewSelectedWindowFlag && (newPt.h == startPt.h) && (newPt.v == startPt.v))
		{
		PenNormal ();
		CheckAndUnlockHandle (mapProjectionHandle);
																									return;
		
		}	// end "if (newPt == startPt)" 
		
			// Get the mouse up event to determine whether the command key			
			// was down.																			
			
	CheckSomeEvents (mUpMask);
		
			// FrameRect would cause the system to crash when 							
			// running heap scramble under macbugs. 										
			// Larry Biehl 2-7-91.																
			
	FrameRect (&gGrayRect);
	/*
	MoveTo (gGrayRect.left,  gGrayRect.top);
	LineTo (gGrayRect.right,  gGrayRect.top);
	LineTo (gGrayRect.right,  gGrayRect.bottom);
	LineTo (gGrayRect.left,  gGrayRect.bottom);
	LineTo (gGrayRect.left,  gGrayRect.top);
	*/
	PenNormal ();
		
			// Invalidate the current selection rectangle if it exists.				
	
	ClearSelectionArea (gActiveImageWindow);
	
	if (coordinateFlag)
		InvalidateWindow (gActiveImageWindow, 
								kCoordinateSelectionArea,
								FALSE);
	
			// If Coordinates view is showing, force the line-column 		
			// values for the current position of the cursor to be displayed.		
	
	gCoordinateLineValue = -1;
	gCoordinateColumnValue = -1;
	UpdateCursorCoordinates (newPt);
	
	ComputeSelectionLineColumns (displaySpecsPtr,
											mapProjectionInfoPtr,
											//hConversionFactor,
											//vConversionFactor,
											&gTempLongRect,
											&gGrayRect,
											&gTempLongPoint,
											&coordinateRect);
								
	SetSelectionInformation (NULL,
										displaySpecsPtr,
										selectionInfoPtr,
										kRectangleType,
										startChannel,
										&gTempLongRect,
										&coordinateRect);
										
	CheckAndUnlockHandle (mapProjectionHandle);
	
}	// end "RectangleSelection" 



//------------------------------------------------------------------------------------
//								 Copyright (1988-2018)
//								(c) Purdue Research Foundation
//									All rights reserved.
//
//	Function name:		void SelectArea
//
//	Software purpose:	This routine handles a mousedown event when the 
//							cursor is a cross, i.e. when there is a mousedown
//							event in an image window.
//
//	Parameters in:		Start point for rectangle selection or another vertex 
//								for a polygon.
//
//	Parameters out:				
//
// Value Returned:	None
// 
// Called By:			DoImageMouseDnContent in multiSpec.c
//							DoThematicMouseDnContent in thematicWindow.c
//
//	Coded By:			Larry L. Biehl			Date: 08/30/1989
//	Revised By:			Larry L. Biehl			Date: 03/17/2005			

void SelectArea  (
				Point									mousePt)

{
	DoubleRect							coordinateRectangle;
	
	double								factor;
	
	DisplaySpecsPtr					displaySpecsPtr;
	SelectionInfoPtr					selectionInfoPtr;
	
	Handle								displaySpecsH,
											selectionInfoH;
											
	SInt16								coordinateViewUnits,
											unitsToUseCode;
											
	Boolean								useStartLineColumnFlag;
	
		
			// Get the pointer to the selection rectangle information.		 		
				
	selectionInfoH = GetSelectionInfoHandle (gActiveImageWindowInfoH);
	selectionInfoPtr = (SelectionInfoPtr)GetHandlePointer (selectionInfoH, kLock);
	
			// Get pointer to display specifications										
	
	displaySpecsH = GetActiveDisplaySpecsHandle ();
	displaySpecsPtr = (DisplaySpecsPtr)GetHandlePointer (displaySpecsH, kLock);
	
	SetPortWindowPort (gActiveImageWindow);
	if (GetProjectWindowFlag (gActiveImageWindowInfoH))
		{
		gProjectSelectionWindow = gActiveImageWindow;
		
		if (gStatisticsMode == kStatNewFieldRectMode)
			{
			RectangleSelection (mousePt, selectionInfoPtr, displaySpecsPtr);
			LoadNewFieldListBox ();
														
			}	// end "if (gStatisticsMode == kStatNewFieldRectMode)" 
	
		//else if (gStatisticsMode == kStatNewFieldPolyMode)
		else if (gStatisticsMode > 0 && gProjectInfoPtr->selectionType == kPolygonType)
			{					
	   	gProcessorCode = kPolygonSelectionProcessor;
			PolygonSelection (mousePt, selectionInfoPtr, displaySpecsPtr);
	   	gProcessorCode = 0;
			
			}	// end "if (gStatisticsMode == kStatNewFieldPolyMode)" 
												
		else	// (gStatisticsMode >= kStatClassListMode || gStatisticsMode == 0)
			RectangleSelection (mousePt, selectionInfoPtr, displaySpecsPtr);
												
		}	// end "if (GetProjectWindowFlag (gActiveImageWindowInfoH))" 
		
	else	// !GetProjectWindowFlag (gActiveImageWindowInfoH)
		RectangleSelection (mousePt, selectionInfoPtr, displaySpecsPtr);
										
			// Outline around the selection.													
			
	OutlineSelectionArea (gActiveImageWindow);
 														
 			// Make certain that the final selected line-columns is in the 		
 			// coordinate window if it exists.  This routine will also update		
 			// the "Clear Selection Rectangle" menu item.  If the selection is	
 			// a polygon then the boundary lines and columns for the selected		
 			// polygon will be displayed in the coordinate window if it exists. 	
 			
 	ShowSelection (gActiveImageWindowInfoH);
 	
 			// Update the controls in the coordinate view.
 	
 	UpdateCoordinateViewControls (gActiveImageWindow);
	
			// Set this selection for all image windows if requested. If the shift key
			// was down when the selection was made, then do not use the start line
			// and column in calculating the selection coordinates in the other windows.
	
	useStartLineColumnFlag = TRUE;
	if (gEventRecord.modifiers & shiftKey)
		useStartLineColumnFlag = FALSE;
	
	if (gEventRecord.modifiers & controlKey)
		{
		unitsToUseCode = kLineColumnUnits;
		
		coordinateViewUnits = GetCoordinateViewUnits (gActiveImageWindowInfoH);
		if (GetCoordinateHeight (gActiveImageWindowInfoH) > 0)
			{
			if (coordinateViewUnits >= kKilometersUnitsMenuItem)
				unitsToUseCode = kMapUnits;
			
			else if (coordinateViewUnits == kDecimalLatLongUnitsMenuItem ||
						 coordinateViewUnits == kDMSLatLongUnitsMenuItem)
				unitsToUseCode = kLatLongUnits;
			
			}	// end "if (GetCoordinateHeight (gActiveImageWindowInfoH) > 0)"
		
		GetSelectionCoordinates (gActiveImageWindow, 
											NULL,
											&coordinateRectangle,
											NULL);
											
				// Get the factor that is being used to convert the original units to
				// the requested display units.
													
		factor = GetCoordinateViewFactor (gActiveImageWindowInfoH);
											
		SetSelectionForAllWindows (gActiveImageWindowInfoH, 
											selectionInfoPtr,
											&coordinateRectangle,
											useStartLineColumnFlag,
											unitsToUseCode,
											factor);
											
		}	// end "if (gEventRecord.modifiers & controlKey)"
	
			//	Unlock handles to display specifications and selection 				
			// information. 																		
							
	HUnlock (displaySpecsH);
	HUnlock (selectionInfoH);
	
	ShowGraphSelection ();

	gUpdateFileMenuItemsFlag = TRUE;
	gUpdateEditMenuItemsFlag = TRUE;
			
}	// end "SelectArea"



//------------------------------------------------------------------------------------
//								 Copyright (1988-2018)
//							(c) Purdue Research Foundation
//									All rights reserved.
//
//	Function name:		void ShowSelection
//
//	Software purpose:	This routine lists the selection rectangle that has been set up
//							in the dragGrayRgn structure for the given window in the
//							selection dialog window.  It will also make certain that the
//							"Clear Selection Rectangle" menu item is correct.
//
//	Parameters in:				
//
//	Parameters out:				
//
// Value Returned:	None
// 
// Called By:			DoImageWActivateEvent in multiSpec.c
//							DoImageWUpdateEvent in multiSpec.c
//							SelectArea in selectionArea.c
//							DoEditSelectAllImage in SMenu.cpp
//							ClearSelectionArea in SSelUtil.cpp
//							DoThematicWUpdateEvent in thematicWindow.c
//							CloseImageWindow in window.c
//
//	Coded By:			Larry L. Biehl			Date: 09/13/1988
//	Revised By:			Larry L. Biehl			Date: 02/09/2001			

void ShowSelection (
				Handle								windowInfoH)

{
	GrafPtr								savedPort;
	SelectionInfoPtr					selectionInfoPtr;
	WindowPtr							windowPtr;
	
	Handle								selectionInfoH;
	
	SignedByte							handleStatus;
	
	
	if (windowInfoH == NULL)
																									return;
	
			// Get pointer to the selection rectangle structure for the given		
			// window.																				
					
	selectionInfoH = ((WindowInfoPtr)*windowInfoH)->selectionInfoHandle;
	if (selectionInfoH == NULL)
																									return;
		
			// Update the "Clear Selection Rectangle" menu item.	
			
	gUpdateEditMenuItemsFlag = TRUE;
		   
	if (GetCoordinateHeight (windowInfoH) > 0)
		{																				
		handleStatus = HGetState (selectionInfoH);
		HLock (selectionInfoH);
		selectionInfoPtr = (SelectionInfoPtr)*selectionInfoH;
		
		if (selectionInfoPtr->typeFlag != 0)
			{
			GetPort (&savedPort);
			windowPtr = ((WindowInfoPtr)*windowInfoH)->windowPtr;
			SetPortWindowPort (windowPtr);
		
			DrawSelectionCoordinates (windowInfoH,
												&selectionInfoPtr->lineColumnRectangle,
												&selectionInfoPtr->coordinateRectangle,
												selectionInfoPtr->numberPixels);
		
			SetPort (savedPort);
			
			}	// end "if (selectionInfoPtr->typeFlag != 0)"
		
		HSetState (selectionInfoH, handleStatus);
		
		}	// end "if (GetCoordinateHeight (windowInfoH) > 0)" 
			
}	// end "ShowSelection" 
