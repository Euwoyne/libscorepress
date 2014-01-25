
/*
  ScorePress - Music Engraving Software  (libscorepress)
  Copyright (C) 2014 Dominik Lehmann
  
  Licensed under the EUPL, Version 1.1 or - as soon they
  will be approved by the European Commission - subsequent
  versions of the EUPL (the "Licence");
  You may not use this work except in compliance with the
  Licence.
  
  Unless required by applicable law or agreed to in
  writing, software distributed under the Licence is
  distributed on an "AS IS" basis, WITHOUT WARRANTIES OR
  CONDITIONS OF ANY KIND, either expressed or implied.
  See the Licence for the specific language governing
  permissions and limitations under the Licence.
*/

#include <iostream>             // std::cout
#include <cmath>                // sqrt

#include "renderer.hh"          // Renderer, Sprites, std::string

using namespace ScorePress;


//
//     class Renderer
//    ================
//
// This is an (partially) abstract renderer interface.
// It is used by the engine to render the prepared score, such that the engine
// is independent of the used frontend.
//

// dump sprite info to stdout
void Renderer::dump() const
{
    for (Sprites::const_iterator s = sprites.begin(); s != sprites.end(); s++)
    for (SpriteSet::const_iterator i = s->begin(); i != s->end(); i++)
    {
        for (std::map<std::string, std::string>::const_iterator t = i->text.begin(); t != i->text.end(); t++)
        {
            std::cout << t->first << "$ = \"" << t->second << "\"\n";
        };
        std::cout << "path$ = \"" << i->path << "\"\n";
        std::cout << "width% = " << i->width << "\n";
        std::cout << "height% = " << i->height << "\n";
        for (std::map<std::string, int>::const_iterator n = i->integer.begin(); n != i->integer.end(); n++)
        {
            std::cout << n->first << "% = " << n->second << "\n";
        };
        for (std::map<std::string, double>::const_iterator r = i->real.begin(); r != i->real.end(); r++)
        {
            std::cout << r->first << "& = " << r->second << "\n";
        };
        std::cout << "\n";
    };
}

// virtual destructor
Renderer::~Renderer() {}

// render a cubic bézier curve
void Renderer::bezier(double  x1, double  y1,
                      double cx1, double cy1,
                      double cx2, double cy2,
                      double  x2, double  y2)
{
    // calculate step
    const double step = 4.0 / (
        sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)) + 
        sqrt((cx1 - x1) * (cx1 - x1) + (cy1 - y1) * (cy1 - y1)) +
        sqrt((cx2 - cx1) * (cx2 - cx1) + (cy2 - cy1) * (cy2 - cy1)) + 
        sqrt((x2 - cx2) * (x2 - cx2) + (y2 - cy2) * (y2 - cy2)));
    
    // render bézier curve
    move_to(x1, y1);    // move to beginning
    for (double t = 0, s = 1; t <= 1 && s >= 0; t += step, s -= step)   // for each segment
    {
        // line to next point
        // B(t) = (1-t)^3 * p1 + 3*(1-t)^2*t * c1 + 3*(1-t)*t^2 * c2 + t^3 * p2
        line_to(s*s*(s*x1 + 3*t*cx1) + t*t*(3*s*cx2 + t*x2),
                s*s*(s*y1 + 3*t*cy1) + t*t*(3*s*cy2 + t*y2));
    };
    line_to(x2, y2);    // move to end
    stroke();           // render the path
}

// render a cubic bézier curve with different thickness in the center than at the ends
void Renderer::bezier_slur(double  x1, double  y1,
                           double cx1, double cy1,
                           double cx2, double cy2,
                           double  x2, double  y2,
                           double  w0, double  w1)
{
    // create local variables
    const double step = 4.0 / ( // calculate step
        sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)) + 
        sqrt((cx1 - x1) * (cx1 - x1) + (cy1 - y1) * (cy1 - y1)) +
        sqrt((cx2 - cx1) * (cx2 - cx1) + (cy2 - cy1) * (cy2 - cy1)) + 
        sqrt((x2 - cx2) * (x2 - cx2) + (y2 - cy2) * (y2 - cy2)));
    double _x1 = x1;        // the previous point (_x1, _y1)
    double _y1 = y1;
    double _x2 = x2;        // the current point (_x2, _y2)
    double _y2 = y2;
    double w = 0;           // half of the current curve-width (absolute value of the offset)
    double l = 0;           // length of the current segment
    
    // render upper bézier curve
    move_to(x1, y1);    // move to beginning
    for (double t = 0, s = 1; t <= 1 && s >= 0; t += step, s -= step)   // for each segment
    {
        // calculate next point
        // B(t) = (1-t)^3 * p1 + 3*(1-t)^2*t * c1 + 3*(1-t)*t^2 * c2 + t^3 * p2
        _x2 = s*s*(s*x1 + 3*t*cx1) + (3*s*cx2 + t*x2)*t*t;
        _y2 = s*s*(s*y1 + 3*t*cy1) + (3*s*cy2 + t*y2)*t*t;
        
        // calculate offset (w = linewidth / 2)
        w = 2 * (w1 - w0) * t * s + w0;
        
        // calculate segment length
        l = sqrt((_x2 - _x1) * (_x2 - _x1) + (_y2 - _y1) * (_y2 - _y1));
        
        // draw segment (add normal if the segment length is positive)
        line_to(_x2 + ((l > 0) ? (_y2 - _y1) * w / l : 0),
                _y2 + ((l > 0) ? (_x1 - _x2) * w / l : 0));
        
        // save point
        _x1 = _x2;
        _y1 = _y2;
    };
    
    // draw final segment
    l = sqrt((_x2 - x2) * (_x2 - x2) + (_y2 - y2) * (_y2 - y2));    // calculate segment length
    line_to(_x2 + ((l > 0) ? (_y2 - y2) * w / l : 0),               // draw segment
            _y2 + ((l > 0) ? (x2 - _x2) * w / l : 0));
    _x1 = x2;           // save end-point
    _y1 = y2;
    
    // render lower bézier curve
    for (double s = 0, t = 1; s <= 1 && t >= 0; s += step, t -= step)   // for each segment
    {   // (swapped s and t, to follow curve backwards)
        
        // calculate next point
        // B(t) = (1-t)^3 * p1 + 3*(1-t)^2*t * c1 + 3*(1-t)*t^2 * c2 + t^3 * p2
        _x2 = s*s*(s*x1 + 3*t*cx1) + (3*s*cx2 + t*x2)*t*t;
        _y2 = s*s*(s*y1 + 3*t*cy1) + (3*s*cy2 + t*y2)*t*t;
        
        // calculate offset (w = linewidth / 2)
        w = 2 * (w1 - w0) * t * s + w0;
        
        // calculate segment length
        l = sqrt((_x2 - _x1) * (_x2 - _x1) + (_y2 - _y1) * (_y2 - _y1));
        
        // draw segment (add negative normal if the segment length is positive)
        line_to(_x2 + ((l > 0) ? (_y2 - _y1) * w / l : 0),
                _y2 + ((l > 0) ? (_x1 - _x2) * w / l : 0));
        
        // save point
        _x1 = _x2;
        _y1 = _y2;
    };
    
    // draw final segment
    l = sqrt((_x2 - x1) * (_x2 - x1) + (_y2 - y1) * (_y2 - y1));    // calculate segment length
    line_to(_x2 + ((l > 0) ? (_y2 - y1) * w / l : 0),               // draw segment
            _y2 + ((l > 0) ? (x1 - _x2) * w / l : 0));
    
    // finish rendering
    fill();         // fill the path
    stroke();       // render the path
}

