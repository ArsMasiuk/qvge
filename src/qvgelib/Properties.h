/*
This file is a part of
QVGE - Qt Visual Graph Editor

(c) 2016-2020 Ars L. Masiuk (ars.masiuk@gmail.com)

It can be used freely, maintaining the information above.
*/

#pragma once


enum PropertiesID
{
	ITEM_ID = 0,	// unique id of an item (text/int)
	ITEM_LABEL,		// label of an item (arbitrary text)
	ITEM_COMMENT,	// any additional text 
	
	ID_USER = 1000	// user-defined properties
};
