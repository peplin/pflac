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

#include "filters.h"

InputFilter::InputFilter( FILE* fin, unsigned s, size_t tot, FLAC::Encoder::File *enc, unsigned int compression_level) :
    count(0),
    filter(true), //serial
    input_file(fin),
    size(s),
    left(tot),
    encoder(enc),
    next_unit(0)
    {
        //initialize units
        for(int i = 0; i < num_tokens; i++){
            unit[i].encoder = enc;
            unit[i].rawBuffer = new FLAC__byte[encoder->get_blocksize()/*samples*/ * 2/*bytes_per_sample*/ * FLAC__MAX_CHANNELS/*channels*/];
            unit[i].buffer = new FLAC__int32[encoder->get_blocksize()/*samples*/ * 2/*bytes_per_sample*/ * FLAC__MAX_CHANNELS/*channels*/];

            unit[i].parallelEncoder = FLAC__stream_encoder_new();
            encoder->parallel_initialize(unit[i].parallelEncoder, compression_level); //copies important metadata

            unit[i].lastBlock = false;
        }
    }

InputFilter::~InputFilter(){
    for(int i = 0; i < num_tokens; i++){
        delete[] unit[i].rawBuffer;
        delete[] unit[i].buffer;

        //even though there are no blocks to finish, must do this to avoid double free
        FLAC__stream_encoder_finish_parallel(unit[i].parallelEncoder);
        FLAC__stream_encoder_delete(unit[i].parallelEncoder);
    }
}

void* InputFilter::operator()(void*) {
    PipelineUnit& u = unit[next_unit];

    need = u.need = (left>encoder->get_blocksize()? (size_t)encoder->get_blocksize() : (size_t)left);
    if(need == left)
        u.lastBlock = true;

    next_unit = (next_unit + 1) % num_tokens;

    ++count;

    /* IMPORTANT: the parallel encoder must read a full block every for every unit except the last
     *            in serial, it's okay to build up a block READSIZE number of samples at a time,
     *            taking 3-4 cycling before processing/writing. We CANNOT do that here because our pipeline
     *            splits the data by block.
     */
    if(!left)
        return NULL;
    if(fread(u.rawBuffer, encoder->get_channels() * (encoder->get_bits_per_sample() / 8), need, input_file) != need) {
        fprintf(stderr, "ERROR: reading from WAVE file\n");
        return NULL;
    } else{
        left -= need;
        return &u;
    }
}

PCMFilter::PCMFilter() :
    filter(false) //parallel
{}

void* PCMFilter::operator()( void* item ) {
    PipelineUnit& u = *static_cast<PipelineUnit*>(item);

 /* convert the packed little-endian 16-bit PCM samples from WAVE into an interleaved FLAC__int32 buffer for libFLAC */
    size_t i;
    for(i = 0; i < u.need * u.encoder->get_channels(); i++) {
        /* inefficient but simple and works on big- or little-endian machines */
        u.buffer[i] = (FLAC__int32)(((FLAC__int16)(FLAC__int8)u.rawBuffer[2*i+1] << 8) | (FLAC__int16)u.rawBuffer[2*i]);
    }

    return &u;
}

ProcessFilter::ProcessFilter() :
    filter(false) //parallel
{}

void* ProcessFilter::operator()( void* item ) {
    PipelineUnit& u = *static_cast<PipelineUnit*>(item);

    u.encoder->process_interleaved_no_write(u.buffer, u.need, u.parallelEncoder, u.lastBlock);

    //u.encoder.parallelEncoder now has a frame buffer ready to write

    return &u;
}

OutputFilter::OutputFilter( ) :
    tbb::filter(/*is_serial=*/true),
    current_frame_number(0)
{
}

void* OutputFilter::operator()( void* item ) {
    PipelineUnit& u = *static_cast<PipelineUnit*>(item);

    //increment frame for seek table
    FLAC__stream_encoder_set_current_frame(u.parallelEncoder, current_frame_number++);

    if(!u.lastBlock){
        if(!u.encoder->write_parallel(u.parallelEncoder)) //no shared encoder - parallelEncoder has everything it needs
            return false;
    }

    return NULL;
}

