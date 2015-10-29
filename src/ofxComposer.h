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
#include "ofxPatch.h"

//  Comment the "define USE_OFXGLEDITOR" if you don't want to use ofxGLEditor
//
//#define USE_OFXGLEDITOR
#ifdef USE_OFXGLEDITOR
#include "ofxGLEditor.h"
#endif

class ofxComposer {
public:
    ofxComposer();
    
    void    save(string _fileConfig = "default");
    void    load(string _fileConfig = "default");
    bool    addPatchFromFile(string _filePath, ofPoint _position);
    bool    addPatchWithOutFile(string _type, ofPoint _position);
    
    int     size(){return patches.size(); };
    ofxPatch* operator[](int _nID){ if ( (_nID != -1) && (patches[_nID] != NULL) ) return patches[_nID]; };
    
    void    update();
    void    draw();
    
    void    setEdit(bool _state){
        bEditMode = _state;
        for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
            it->second->bEditMode = bEditMode;
        }
    }
    
protected:
    map<int,ofxPatch*>  patches;
    int     getPatchesLowestCoord();
    int     getPatchesHighestCoord();
    
    void    movePatches(ofVec3f diff);
    void    scalePatches(float yDiff);
    void    setDraggingGrip(bool dragging);
    void    setMouseOverGrip(bool over);
    bool    isDraggingGrip();
    bool    isMouseOverGrip();
    
private:
    // Events
    void    _mouseMoved(ofMouseEventArgs &e);
    void    _keyPressed(ofKeyEventArgs &e);
	void    _mousePressed(ofMouseEventArgs &e);
	void    _mouseReleased(ofMouseEventArgs &e);
	void    _windowResized(ofResizeEventArgs &e);
    void    _mouseDragged(ofMouseEventArgs &e);
    
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

#ifdef USE_OFXGLEDITOR
	ofxGLEditor editor;
    ofFbo       editorFbo;
    ofColor     editorBgColor;
    ofColor     editorFgColor;
#endif
    
    string  configFile;
    
    int     selectedDot;
    int     selectedID;
    
    bool    bEditMode;
    bool    bGLEditorPatch, bHelp;
    
    // nico scroll bar
    bool draggingGrip;
    bool mouseOverGrip;
    // nico scroll bar fin
    
};


#endif
