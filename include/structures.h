//
//  structures.h
//  dbio
//
//  Created by Simon Gornall on 7/18/20.
//  Copyright © 2020 Simon Gornall. All rights reserved.
//

#ifndef structures_h
#define structures_h

#include <map>

#include <stdint.h>

/*****************************************************************************\
|* Genereric rectangle structure
\*****************************************************************************/
struct Rect
    {
    int x;              // Top left x
    int y;              // Top left y
    int w;              // Width
    int h;              // Height
 
    void dump(void)
        {
        printf("%d,%d +-> %d,%d\n", x, y, w, h);
        }
    };

/*****************************************************************************\
|* Genereric colour structure
\*****************************************************************************/
struct RGB
    {
    uint8_t r;          // Red component
    uint8_t g;          // Green component
    uint8_t b;          // Blue component

    RGB(uint8_t rv, uint8_t gv, uint8_t bv)
        {
        r = (rv & 0x3F) << 2;
        g = (gv & 0x3f) << 2;
        b = (bv & 0x3F) << 2;
        }

    };


/*****************************************************************************\
|* Used to iterate over a map using the for: approach, as in:
|*
|*  for (Elements<int, std::string> kv : my_map )
|*		{
|*		std::cout << kv.key << " --> " << kv.value;
|*		}
\*****************************************************************************/
template <class K, class T>
struct Elements {
    K const& key;
    T& value;
 
    Elements(std::pair<K const, T>& pair)
        : key(pair.first)
        , value(pair.second)
    { }
};

#endif /* structures_h */
