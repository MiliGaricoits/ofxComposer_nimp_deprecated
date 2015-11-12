//
//  ofxComposer.h
//  GPUBurner
//
//  Created by Patricio Gonzalez Vivo on 3/9/12.
//  Copyright (c) 2012 http://www.PatricioGonzalezVivo.com All rights reserved.
//

#ifndef OFXCOMPOSER
#define OFXCOMPOSER

#include "ofMain.h"
#include "patch.h"

//  Comment the "define USE_OFXGLEDITOR" if you don't want to use ofxGLEditor
//
//#define USE_OFXGLEDITOR
#ifdef USE_OFXGLEDITOR
#include "ofxGLEditor.h"
#endif

class ofxComposer : public ofNode {
public:
    ofxComposer();
    
    void    save(string _fileConfig = "default");
    void    load(string _fileConfig = "default");
    bool    addPatchFromFile(string _filePath, ofPoint _position);
    bool    addPatchWithOutFile(string _type, ofPoint _position);
    
    int     size(){return patches.size(); };
    patch*  operator[](int _nID){ if ( (_nID != -1) && (patches[_nID] != NULL) ) return patches[_nID]; };
    
    void    update();
    void    draw();
    
    void    setEdit(bool _state){
        bEditMode = _state;
        for(map<int,patch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
            it->second->bEditMode = bEditMode;
        }
    }
    
    
    //mili
    void setMainCanvas(ofxUISuperCanvas* gui);
    //
    
protected:
    map<int,patch*>  patches;
    int     getPatchesLowestCoord();
    int     getPatchesHighestCoord();
    int     getPatchesLeftMostCoord();
    int     getPatchesRightMostCoord();
    
    void    movePatches(ofVec3f diff);
    void    scalePatches(float yDiff);
    void    setDraggingGrip(bool dragging);
    void    setDraggingHGrip(bool dragging);
    bool    isDraggingGrip();
    bool    isDraggingHGrip();
    
    // Events
    void    _mouseMoved(ofMouseEventArgs &e);
    void    _keyPressed(ofKeyEventArgs &e);
    void    _mousePressed(ofMouseEventArgs &e);
    void    _mouseReleased(ofMouseEventArgs &e);
    void    _windowResized(ofResizeEventArgs &e);
    void    _mouseDragged(ofMouseEventArgs &e);
    
    void    deactivateAllPatches();
    
    //mili
    ofxUISuperCanvas* gui;
    //
    bool    bEditMode;
    
private:
    void    closePatch( int &_nID );
    void    activePatch( int _nID );
    bool    connect( int _fromID, int _toID, int _nTexture );
    
    // nico Auxiliar Zoom y Drag
    static const float ZOOM_UNIT = 1.f;
    static const float ZOOM_SENSITIVITY = .001f;
    bool    disabledPatches;
    // nico
    int    isAnyPatchHit(float x, float y);
    //
    // mili align nodes
    int verticalAlign1, verticalAlign2, verticalAlign3, horizontalAlign1, horizontalAlign2, horizontalAlign3;
    //
    
    // nico multiple select
    ofRectangle multipleSelectRectangle;
    int     multipleSelectFromX;
    int     multipleSelectFromY;
    void    multipleSelectAndReset();
    bool    isAnyLinkHit();
    

#ifdef USE_OFXGLEDITOR
	ofxGLEditor editor;
    ofFbo       editorFbo;
    ofColor     editorBgColor;
    ofColor     editorFgColor;
#endif
    
    string  configFile;
    
    int     selectedDot;
    int     selectedID;
    
    bool    bGLEditorPatch, bHelp;
    
    // nico scroll bar
    bool draggingGrip;
    bool draggingHGrip;
    // nico scroll bar fin
    
};


#endif
