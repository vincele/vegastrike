#include <assert.h>
#include "networking/lowlevel/netbuffer.h"
#include "networking/lowlevel/vsnet_oss.h"
#include "networking/const.h"
#include "posh.h"
#include "gfxlib_struct.h"
std::string getSimpleString(std::string &input){
  std::string::size_type where=input.find(" ");
  int len=getSimpleInt(input);
  if (len>=0&&len<=input.length()){
    std::string retval=input.substr(0,len);
    input=input.substr(len);
    return retval;
  }
  return "";
}
char getSimpleChar(std::string &input){
  char retval=input[0];
  input=input.substr(1);
  return retval;
}
int getSimpleInt(std::string &input){
  std::string::size_type where=input.find(" ");
  if (where!=string::npos) {
    std::string num=input.substr(0,where);
    int len=XMLSupport::parse_int(num);
    input=input.substr(where+1);
    return len;
  }
  return 0;  
}
void addSimpleString(std::string &input, const std::string adder){
  addSimpleInt(input,adder.length());
  input+=adder;
}
void addSimpleChar(std::string &input, const char adder){
  char add[2]={adder,'\0'};
  input+=std::string(add,1);
}
void addSimpleInt(std::string &input, const int adder){
  input+=XMLSupport::tostring(adder)+" ";
}

NetBuffer::NetBuffer()
		{
			buffer = new char[MAXBUFFER];
			offset = 0;
			size = MAXBUFFER;
			memset( buffer, 0x20, size);
			this->buffer[size-1] = 0;
		}
NetBuffer::NetBuffer( int bufsize)
		{
			buffer = new char[bufsize];
			offset = 0;
			size = bufsize;
			memset( buffer, 0x20, size);
			this->buffer[size-1] = 0;
		}

/** If there is a platform where b in the call VsnetOSS::memcpy(a,b,c) must be char*
 *  instead of const char* or const void*, that systems header files are
 *  badly broken. That requirement must be a lie.
 *  If you have that, implement an ifdef in VsnetOSS::VsnetOSS::memcpy
 */
NetBuffer::NetBuffer( const char * buf, int bufsize)
		{
			offset = 0;
			size=bufsize+1;
			this->buffer = new char[size];
			memset( buffer, 0x20, size);
			VsnetOSS::memcpy( buffer, buf, bufsize);
			this->buffer[size-1] = 0;
		}
NetBuffer::~NetBuffer()
		{
			if( buffer != NULL)
				delete []buffer;
		}
void	NetBuffer::Reset()
		{
			memset( buffer, 0x20, size);
			offset=0;
		}

char *	NetBuffer::getData() { return buffer;}

		// Extends the buffer if we exceed its size
void	NetBuffer::resizeBuffer( int newsize)
		{
			if( size-1 < newsize)
			{
				char * tmp = new char [newsize+1];
				memset( tmp, 0, newsize+1);
				VsnetOSS::memcpy( tmp, buffer, size);
				delete []buffer;
				buffer = tmp;
				size = newsize+1;
			}
		}
// Check the buffer to see if we can still get info from it
bool	NetBuffer::checkBuffer( int len, const char * fun)
{
	if( offset+len >= size)
	{
		std::cerr<<"!!! ERROR : trying to read more data than buffer size (offset="<<offset<<" - size="<<size<<" - to read="<<len<<") in "<<fun<<" !!!"<<std::endl;
		return false;
	}
	return true;
}

// NOTE : IMPORTANT - I ONLY INCREMENT OFFSET IN PRIMARY DATATYPES SINCE ALL OTHER ARE COMPOSED WITH THEM
void	NetBuffer::addClientState( ClientState cs)
{
    //this->addFloat( cs.delay);
    this->addSerial( cs.client_serial);
    this->addTransformation( cs.pos);
    this->addVector( cs.veloc);
    this->addVector( cs.angveloc);
}

ClientState NetBuffer::getClientState()
{
    ClientState cs;
    //cs.delay = this->getFloat();
    cs.client_serial = this->getSerial();
    cs.pos = this->getTransformation();
    cs.veloc = this->getVector();
    cs.angveloc = this->getVector();

    return cs;
}

void	NetBuffer::addVector( Vector v)
		{
			this->addFloat( v.i);
			this->addFloat( v.j);
			this->addFloat( v.k);
		}
Vector	NetBuffer::getVector()
		{
			Vector v;
			v.i = this->getFloat();
			v.j = this->getFloat();
			v.k = this->getFloat();

			return v;
		}
void	NetBuffer::addQVector( QVector v)
		{
			this->addDouble( v.i);
			this->addDouble( v.j);
			this->addDouble( v.k);
		}
QVector	NetBuffer::getQVector()
		{
			QVector v;
			v.i = this->getDouble();
			v.j = this->getDouble();
			v.k = this->getDouble();

			return v;
		}
void	NetBuffer::addColor( GFXColor col)
		{
			this->addFloat( col.r);
			this->addFloat( col.g);
			this->addFloat( col.b);
			this->addFloat( col.a);
		}
GFXColor NetBuffer::getColor()
		{
			GFXColor col;
			col.r = this->getFloat();
			col.g = this->getFloat();
			col.b = this->getFloat();
			col.a = this->getFloat();

			return col;
		}
void	NetBuffer::addMatrix( Matrix m)
		{
			for( int i=0; i<9; i++)
				this->addFloat( m.r[i]);
			this->addQVector( m.p);
		}
Matrix	NetBuffer::getMatrix()
		{
			Matrix m;
			for( int i=0; i<9; i++)
				m.r[i] = this->getFloat();
			m.p = this->getQVector();

			return m;
		}
void	NetBuffer::addQuaternion( Quaternion quat)
		{
			this->addFloat( quat.s);
			this->addVector( quat.v);
		}
Quaternion	NetBuffer::getQuaternion()
		{
			Quaternion q;

			q.s = this->getFloat();
			q.v = this->getVector();

			return q;
		}
void	NetBuffer::addTransformation( Transformation trans)
		{
			this->addQuaternion( trans.orientation);
			this->addQVector( trans.position);
		}
Transformation	NetBuffer::getTransformation()
		{
			Transformation t;
			t.orientation = this->getQuaternion();
			t.position = this->getQVector();

			return t;
		}

void	NetBuffer::addShield( Shield shield)
{
	this->addChar( shield.number);
	this->addChar( shield.leak);
	this->addFloat( shield.recharge);
	this->addFloat( shield.efficiency );
	switch( shield.number)
	{
		case 2 :
			this->addFloat( shield.shield2fb.front);	
			this->addFloat( shield.shield2fb.back);	
			this->addFloat( shield.shield2fb.frontmax);	
			this->addFloat( shield.shield2fb.backmax);	
		break;
		case 4 :
			this->addFloat( shield.shield4fbrl.front);
			this->addFloat( shield.shield4fbrl.back);
			this->addFloat( shield.shield4fbrl.right);
			this->addFloat( shield.shield4fbrl.left);
			this->addFloat( shield.shield4fbrl.frontmax);
			this->addFloat( shield.shield4fbrl.backmax);
			this->addFloat( shield.shield4fbrl.rightmax);
			this->addFloat( shield.shield4fbrl.leftmax);
		break;
		case 8 :
			this->addFloat( shield.shield8.frontrighttop);
			this->addFloat( shield.shield8.backrighttop);
			this->addFloat( shield.shield8.frontlefttop);
			this->addFloat( shield.shield8.backlefttop);
			this->addFloat( shield.shield8.frontrightbottom);
			this->addFloat( shield.shield8.backrightbottom);
			this->addFloat( shield.shield8.frontleftbottom);
			this->addFloat( shield.shield8.backleftbottom);
			this->addFloat( shield.shield8.frontrighttopmax);
			this->addFloat( shield.shield8.backrighttopmax);
			this->addFloat( shield.shield8.frontlefttopmax);
			this->addFloat( shield.shield8.backlefttopmax);
			this->addFloat( shield.shield8.frontrightbottommax);
			this->addFloat( shield.shield8.backrightbottommax);
			this->addFloat( shield.shield8.frontleftbottommax);
			this->addFloat( shield.shield8.backleftbottommax);
		break;
	}
}
Shield	NetBuffer::getShield()
{
	Shield shield;
	shield.number = this->getChar();
	shield.leak = this->getChar();
	shield.recharge = this->getFloat();
	shield.efficiency = this->getFloat();
	switch( shield.number)
	{
		case 2 :
			shield.shield2fb.front=this->getFloat();
			shield.shield2fb.back=this->getFloat();
			shield.shield2fb.frontmax=this->getFloat();
			shield.shield2fb.backmax=this->getFloat();
		break;
		case 4 :
			shield.shield4fbrl.front = this->getFloat();
			shield.shield4fbrl.back = this->getFloat();
			shield.shield4fbrl.right = this->getFloat();
			shield.shield4fbrl.left = this->getFloat();
			shield.shield4fbrl.frontmax = this->getFloat();
			shield.shield4fbrl.backmax = this->getFloat();
			shield.shield4fbrl.rightmax = this->getFloat();
			shield.shield4fbrl.leftmax = this->getFloat();
		break;
		case 8 :
			shield.shield8.frontrighttop=this->getFloat();
			shield.shield8.backrighttop=this->getFloat();
			shield.shield8.frontlefttop=this->getFloat();
			shield.shield8.backlefttop=this->getFloat();
			shield.shield8.frontrightbottom=this->getFloat();
			shield.shield8.backrightbottom=this->getFloat();
			shield.shield8.frontleftbottom=this->getFloat();
			shield.shield8.backleftbottom=this->getFloat();
			shield.shield8.frontrighttopmax=this->getFloat();
			shield.shield8.backrighttopmax=this->getFloat();
			shield.shield8.frontlefttopmax=this->getFloat();
			shield.shield8.backlefttopmax=this->getFloat();
			shield.shield8.frontrightbottommax=this->getFloat();
			shield.shield8.backrightbottommax=this->getFloat();
			shield.shield8.frontleftbottommax=this->getFloat();
			shield.shield8.backleftbottommax=this->getFloat();
		break;
	}

	return shield;
}
void		NetBuffer::addArmor( Armor armor)
{
	this->addFloat ( armor.frontrighttop);
	this->addFloat( armor.backrighttop);
	this->addFloat( armor.frontlefttop);
	this->addFloat( armor.backlefttop);
	this->addFloat ( armor.frontrightbottom);
	this->addFloat( armor.backrightbottom);
	this->addFloat( armor.frontleftbottom);
	this->addFloat( armor.backleftbottom);
}
Armor	NetBuffer::getArmor()
{
	Armor armor;
	armor.frontrighttop = this->getFloat();
	armor.backrighttop = this->getFloat();
	armor.frontlefttop = this->getFloat();
	armor.backlefttop = this->getFloat();
	armor.frontrightbottom = this->getFloat();
	armor.backrightbottom = this->getFloat();
	armor.frontleftbottom = this->getFloat();
	armor.backleftbottom = this->getFloat();

	return armor;
}

void	NetBuffer::addSerial( ObjSerial serial)
		{
			this->addShort( serial);
		}
ObjSerial	NetBuffer::getSerial()
		{
			return this->getShort();
		}
void	NetBuffer::addFloat( float f)
		{
			int tmpsize = sizeof( f);
			resizeBuffer( offset+tmpsize);
			posh_u32_t bits = POSH_BigFloatBits( f );
			*((posh_u32_t*)(this->buffer+offset)) = bits;
			offset += tmpsize;
		}
float	NetBuffer::getFloat()
		{
			float s;
			if (!checkBuffer( sizeof( s), "getFloat"))
				return 0;
			posh_u32_t bits = *((posh_u32_t*)(this->buffer+offset));
			s = POSH_FloatFromBigBits( bits );
			offset+=sizeof(s);
			return s;
		}
void	NetBuffer::addDouble( double d)
		{
			int tmpsize = sizeof( d);
			resizeBuffer( offset+tmpsize);
			POSH_DoubleBits( d, (posh_byte_t *) this->buffer+offset);
			offset += tmpsize;
		}
double	NetBuffer::getDouble()
		{
			double s;
			if (!checkBuffer( sizeof( s), "getDouble"))
				return 0;
			s = POSH_DoubleFromBits( (posh_byte_t *) this->buffer+offset);
			offset+=sizeof(s);
			return s;
		}
void	NetBuffer::addShort( unsigned short s)
		{
			int tmpsize = sizeof( s);
			resizeBuffer( offset+tmpsize);
			POSH_WriteU16ToBig( this->buffer+offset, s);
			offset += tmpsize;
		}
unsigned short	NetBuffer::getShort()
		{
			unsigned short s;
			if (!checkBuffer( sizeof( s), "getShort"))
				return 0;
			s = POSH_ReadU16FromBig( this->buffer+offset);
			//cerr<<"getShort :: offset="<<offset<<" - length="<<sizeof( s)<<" - value="<<s<<endl;
			offset+=sizeof(s);
			return s;
		}
void	NetBuffer::addInt32( int i)
		{
			int tmpsize = sizeof( i);
			resizeBuffer( offset+tmpsize);
			POSH_WriteS32ToBig( this->buffer+offset, i);
			offset += tmpsize;
		}
int		NetBuffer::getInt32()
		{
			int s;
			if (!checkBuffer( sizeof( s), "getInt32"))
				return 0;
			s = POSH_ReadS32FromBig( this->buffer+offset);
			offset+=sizeof(s);
			return s;
		}
void	NetBuffer::addUInt32( unsigned int i)
		{
			int tmpsize = sizeof( i);
			resizeBuffer( offset+tmpsize);
			POSH_WriteU32ToBig( this->buffer+offset, i);
			offset += tmpsize;
		}
unsigned int	NetBuffer::getUInt32()
		{
			unsigned int s;
			if (!checkBuffer( sizeof( s), "getUInt32"))
				return 0;
			s = POSH_ReadU32FromBig( this->buffer+offset);
			offset+=sizeof(s);
			return s;
		}
void	NetBuffer::addChar( char c)
		{
			int tmpsize = sizeof( c);
			resizeBuffer( offset+tmpsize);
			VsnetOSS::memcpy( buffer+offset, &c, sizeof( c));
			offset += tmpsize;
		}
char	NetBuffer::getChar()
		{
			char c;
			if (!checkBuffer( sizeof( c), "getChar"))
				return 0;
			VsnetOSS::memcpy( &c, buffer+offset, sizeof( c));
			offset+=sizeof(c);
			return c;
		}
void	NetBuffer::addBuffer( const unsigned char * buf, int bufsize)
		{
			resizeBuffer( offset+bufsize);
			VsnetOSS::memcpy( buffer+offset, buf, bufsize);
			offset+=bufsize;
		}
unsigned char* NetBuffer::extAddBuffer( int bufsize )
		{
			resizeBuffer( offset+bufsize);
            unsigned char* retval = (unsigned char*)buffer+offset;
			offset+=bufsize;
            return retval;
		}

static unsigned char null = '\0';

unsigned char *	NetBuffer::getBuffer( int offt)
		{
			if (!checkBuffer( offt, "getBuffer"))
				return &null;
			unsigned char * tmp = (unsigned char *)buffer + offset;
			offset += offt;
			return tmp;
		}
		// Add and get a string with its length before the char * buffer part
void	NetBuffer::addString( const string& str)
{
	unsigned int len = str.length();
    if( len < 0xffff )
	{
        unsigned short length = len;
        this->addShort( length );
        resizeBuffer( offset+length );
        VsnetOSS::memcpy( buffer+offset, str.c_str(), length );
        offset += length;
	}
	else
	{
		assert( len < 0xffffffff );
        this->addShort( 0xffff );
		this->addInt32( len );
        resizeBuffer( offset+len );
        VsnetOSS::memcpy( buffer+offset, str.c_str(), len );
        offset += len;
	}
}

string	NetBuffer::getString()
{
	unsigned short s;
	s = this->getShort();
	if( s != 0xffff )
	{
		if (!checkBuffer( s, "getString"))
			return std::string();
		char c = buffer[offset+s];
		buffer[offset+s]=0;
	    string str( buffer+offset);
	    buffer[offset+s]=c;
	    offset += s;

	    return str;
	}
	else
	{
		// NETFIXME: Possible DOS attack here...
		unsigned int len = this->getInt32();
		if (!checkBuffer( len, "getString"))
			return std::string();
		char c = buffer[offset+len];
		buffer[offset+len]=0;
	    string str( buffer+offset);
	    buffer[offset+len]=c;
	    offset += len;

	    return str;
	}
}

GFXMaterial		NetBuffer::getGFXMaterial()
{
	GFXMaterial mat;

	mat.ar = this->getFloat();
	mat.ag = this->getFloat();
	mat.ab = this->getFloat();
	mat.aa = this->getFloat();

	mat.dr = this->getFloat();
	mat.dg = this->getFloat();
	mat.db = this->getFloat();
	mat.da = this->getFloat();

	mat.sr = this->getFloat();
	mat.sg = this->getFloat();
	mat.sb = this->getFloat();
	mat.sa = this->getFloat();

	mat.er = this->getFloat();
	mat.eg = this->getFloat();
	mat.eb = this->getFloat();
	mat.ea = this->getFloat();

	mat.power = this->getFloat();

	return mat;
}

void	NetBuffer::addGFXMaterial( const GFXMaterial & mat)
{
	this->addFloat( mat.ar);
	this->addFloat( mat.ag);
	this->addFloat( mat.ab);
	this->addFloat( mat.aa);

	this->addFloat( mat.dr);
	this->addFloat( mat.dg);
	this->addFloat( mat.db);
	this->addFloat( mat.da);

	this->addFloat( mat.sr);
	this->addFloat( mat.sg);
	this->addFloat( mat.sb);
	this->addFloat( mat.sa);

	this->addFloat( mat.er);
	this->addFloat( mat.eg);
	this->addFloat( mat.eb);
	this->addFloat( mat.ea);

	this->addFloat( mat.power);
}

GFXLight		NetBuffer::getGFXLight()
{
	GFXLight	light;
	int i=0;
	light.target = this->getInt32();
	for( i=0; i<3; i++)
		light.vect[i] = this->getFloat();
	light.options = this->getInt32();
	for( i=0; i<4; i++)
		light.diffuse[i] = this->getFloat();
	for( i=0; i<4; i++)
		light.specular[i] = this->getFloat();
	for( i=0; i<4; i++)
		light.ambient[i] = this->getFloat();
	for( i=0; i<3; i++)
		light.attenuate[i] = this->getFloat();
	for( i=0; i<3; i++)
		light.direction[i] = this->getFloat();
	light.exp = this->getFloat();
	light.cutoff = this->getFloat();

	return light;
}

void	NetBuffer::addGFXLight( const GFXLight & light)
{
	int i=0;
	this->addInt32( light.target);
	for( i=0; i<3; i++)
		this->addFloat( light.vect[i]);
	this->addInt32( light.options);
	for( i=0; i<4; i++)
		this->addFloat( light.diffuse[i]);
	for( i=0; i<4; i++)
		this->addFloat( light.specular[i]);
	for( i=0; i<4; i++)
		this->addFloat( light.ambient[i]);
	for( i=0; i<3; i++)
		this->addFloat( light.attenuate[i]);
	for( i=0; i<3; i++)
		this->addFloat( light.direction[i]);
	this->addFloat( light.exp);
	this->addFloat( light.cutoff);
}

GFXLightLocal	NetBuffer::getGFXLightLocal()
{
	GFXLightLocal light;

	light.ligh = this->getGFXLight();
	light.islocal = this->getChar();

	return light;
}

void	NetBuffer::addGFXLightLocal( const GFXLightLocal & light)
{
	this->addGFXLight( light.ligh);
	this->addChar( light.islocal);
}

int		NetBuffer::getDataLength() { return offset;}
int		NetBuffer::getSize() { return size;}