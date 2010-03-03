#include "../../first.h"
#include "../mem_texture.h"


mem_texture::mem_texture()
{
    //initialize the angles from center
    double inv_factor = 1.0 / double( SIZE >> 1 );
    for( size_t i = 0; i < SIZE>>1; ++i )
    {
        double tan_angle = i * inv_factor;
        double r = atan( tan_angle );
        angle_from_center_[i] = radians( r );
    }
}

size_t mem_texture::tex_index( side const & s, icoords const & ic ) const
{
    return ( s.get_num() * SIZE*SIZE*3 )
         + ( ic.v_ * SIZE*3 )
         + ( ic.u_ * 3 );
}

radians mem_texture::angle_from_center( size_t single_coord ) const
{
    //suppose it was an 8 x 8 texture: i is 0 to 7.
    //m (mid point) is 4; so we need the average of angles for
    //abs(i-4) and abs(i-3)...
    int    i = int( single_coord );
    int    m = int( SIZE >> 1 );
    size_t a = size_t( abs( i-m ) );
    size_t b = size_t( abs( i+1-m ) );
    return 0.5f * ( angles_from_center_[a] + angles_from_center_[b] );
}

radians mem_texture::texel_angle_width( size_t single_coord ) const
{
    //this is similar, but we need the absolute difference
    int    i = int( single_coord );
    int    m = int( SIZE >> 1 );
    size_t a = size_t( abs( i-m ) );
    size_t b = size_t( abs( i+1-m ) );
    return abs( angles_from_center_[a] - angles_from_center_[b] );
}

steradians mem_texture::texel_solid_angle( icoords const & ic ) const
{
    return texel_angle_width( ic.u_ ) * texel_angle_width( ic.v_ );
}

tex_fetch_t mem_texture::fetch_texel( side const & s, icoords const & ic ) const
{
    size_t i = tex_index( s, ic );
    fcolor fc( buff[i], buff[++i], buff[++i] );
    return tex_fetch_t( fc, texel_solid_angle( ic ) );
}

