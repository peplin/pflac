/* pflac - command line parallel andio encoder, using libFLAC and libFLAC++
 * Developed by Christopher Peplin & Maxwell Miller, 2008
 * University of Michigan
 * peplin@umich.edu, mgmiller@umich.edu
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

/* PipelineUnit.h - a token, or unit, in the TBB pipeline for the example encoder
 * Used by filters.h
 */

#include "FLAC++/encoder.h"

typedef struct {
    FLAC::Encoder::File *encoder; //shared encoder, essentially const except for metadata, # frames written, and final block
    FLAC__StreamEncoder *parallelEncoder; //use buffers here for parallel processing

    FLAC__byte *rawBuffer;
    FLAC__int32 *buffer; //rawBuffer after PCM conversion

    unsigned need; //amount needed for a block, const except for last block which may be variable size
    bool lastBlock; //processing last block, don't do it parallel

} PipelineUnit;
