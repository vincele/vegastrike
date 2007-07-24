/* 
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
 * You should have recvbufd a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/*
  client structures - written by Stephane Vaxelaire <svax@free.fr>
*/

#ifndef VSNET_CLIENTSTATE_H
#define VSNET_CLIENTSTATE_H

#include <iostream>
#include <string.h>
#include "gfx/quaternion.h"
#include "networking/const.h"
#include "configxml.h"

class Unit;
class NetBuffer;

// Structure used to transmit client updates
class	ClientState
{
	//float			delay;
	ObjSerial		client_serial;
	Transformation	pos;
	Vector			veloc;
  //NO longer supproted--wasnt indicative of actual accel	Vector			accel;
	Vector			angveloc;
	public:
		ClientState();
		ClientState( ObjSerial serial);
		ClientState( ObjSerial serial, QVector posit, Quaternion orientat, Vector velocity, Vector acc, Vector angvel);
		ClientState( ObjSerial serial, QVector posit, Quaternion orientat, Vector velocity, Vector acc, Vector angvel, unsigned int del);
		ClientState( ObjSerial serial, Transformation trans, Vector velocity, Vector acc, Vector angvel, unsigned int del);
		ClientState( Unit * un);

		QVector		getPosition() const { return this->pos.position;}
		Quaternion	getOrientation() const { return this->pos.orientation;}
		Vector		getVelocity() const { return this->veloc;}
		Vector		getAngularVelocity() const { return this->angveloc;}
  //NO longer supported--wasn't indicative of actual aggregated accel		Vector		getAcceleration() const { retu //rn if you change this, change setAcceleration too, and all consturctor this->accel;}
                void		setAcceleration( Vector acc) { }

		ObjSerial	getSerial() const { return this->client_serial;}
		//float		getDelay() const { return this->delay;}
		//void		setDelay( float del) { this->delay = del;}
		void		setSerial( ObjSerial ser) { this->client_serial = ser;}
		void		setPosition( QVector posit) { this->pos.position = posit;}
		void		setOrientation( Quaternion orient) { this->pos.orientation = orient;}
		void		setVelocity( Vector vel) { this->veloc = vel;}
		void		setAngularVelocity( Vector vel) { this->angveloc = vel;}

		void	display( std::ostream& ostr ) const;
		void	display() const;
		int		operator==( const ClientState & ctmp) const;
		void	netswap();

		friend	class NetBuffer;
};

std::ostream& operator<<( std::ostream& ostr, const ClientState& cs );

#endif
