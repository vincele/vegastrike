/*
 * Vega Strike
 * Copyright (C) 2001-2002 Daniel Horn & Alan Shieh
 *
 * http://vegastrike.sourceforge.net/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */


#include "matrix.h"
#include "gfxlib.h"
#include "physics.h"

#ifndef _CAMERA_H_
#define _CAMERA_H_
class PlanetaryTransform;
#include "cmd/container.h"
class Nebula;
class Camera{
  
	QVector Coord;
	Vector velocity;
	Vector angular_velocity;
    Vector accel;
	Matrix planetview;
	GFXBOOL changed;
	QVector lastpos;
	float x, y, xsize, ysize;
	float zoom;
    float fov;
	float cockpit_offset;
        UnitContainer nebula;
public:
	void setCockpitOffset (float c) {cockpit_offset = c;changed=GFXTRUE;}
	float getCockpitOffset () {return cockpit_offset;} 
	enum ProjectionType {
	  PARALLEL,
	  PERSPECTIVE
	};
	
private:
	ProjectionType projectionType;
	PlanetaryTransform * planet;

private:
    // Last GFX Update, for partial updates
    struct LastGFXUpdateStruct {
        GFXBOOL clip;
        GFXBOOL updateFrustum;
        GFXBOOL centerCamera;
        GFXBOOL overrideZFrustum;
        float overrideZNear, overrideZFar;
    } lastGFXUpdate;

public:
	void LookDirection(const Vector &forevec, const Vector &up);
	Vector P,Q,R;
	void SetNebula(Nebula * neb);
	Nebula * GetNebula();
	PlanetaryTransform * GetPlanetaryTransform() {return planet;}
	void SetPlanetaryTransform(PlanetaryTransform * t) {planet=t;}
	PhysicsSystem myPhysics;
	///This function updates the sound if sound is not updated on a per frame basis
	void UpdateCameraSounds();
	Camera(ProjectionType proj = PERSPECTIVE);
        void GetView (Matrix &);
        const Vector & GetR () {return R;}
	void GetPQR (Vector &p1, Vector &q1, Vector &r1);
	void UpdateGFX(GFXBOOL clip, GFXBOOL updateFrustum, GFXBOOL centerCamera, GFXBOOL overrideZFrustum, float overrideZNear, float overrideZFar);
    void UpdateGFX(GFXBOOL clip = GFXTRUE, GFXBOOL updateFrustum=GFXTRUE, GFXBOOL centerCamera = GFXFALSE) { UpdateGFX(clip,updateFrustum,centerCamera,lastGFXUpdate.overrideZFrustum,lastGFXUpdate.overrideZNear,lastGFXUpdate.overrideZFar); };
    void UpdateGFXFrustum(GFXBOOL overrideZFrustum, float overrideZNear, float overrideZFar) { UpdateGFX(lastGFXUpdate.clip,lastGFXUpdate.updateFrustum,lastGFXUpdate.centerCamera,overrideZFrustum,overrideZNear,overrideZFar); };
    void UpdateGFXAgain() { UpdateGFX(lastGFXUpdate.clip,lastGFXUpdate.updateFrustum,lastGFXUpdate.centerCamera,lastGFXUpdate.overrideZFrustum,lastGFXUpdate.overrideZNear,lastGFXUpdate.overrideZFar); };
	void UpdatePlanetGFX();//clip true, frustum true at all times
	Matrix * GetPlanetGFX();
	void UpdateGLCenter();

	void SetPosition(const QVector &origin, const Vector & velocity, const Vector & angular_velocity, const Vector & acceleration);
	void GetPosition(QVector &vect) {vect=Coord;}
	Vector GetAngularVelocity() const{return angular_velocity;}
	Vector GetVelocity() const{return velocity;}
    Vector GetAcceleration() const { return accel; }
	void GetOrientation(Vector &p, Vector &q, Vector &r) {p=P;q=Q;r=R;}
	const QVector &GetPosition() { return Coord;}

    float GetZDist(const Vector &v) const { return ::DotProduct(QVector(v)-Coord,QVector(R));};

	void SetOrientation(const Vector &p, const Vector &q, const Vector &r);
	void SetSubwindow(float x, float y, float xsize, float ysize);
	void SetProjectionType(ProjectionType t);
	void SetZoom(float z);
	float GetZoom();
    void SetFov(float f);
    float GetFov();
	void Yaw(float rad);
	void Pitch(float rad);
	void Roll(float rad);
	void XSlide(float factor);
	void YSlide(float factor);
	void ZSlide(float factor);
	void RestoreViewPort(float xoffset, float yoffset);
};

#endif
