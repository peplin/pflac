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

/*
 * This file holds the filter classes for the TBB pipeline used in the pflac encoder.
 *
 */


#include "tbb/pipeline.h"
#include "FLAC++/encoder.h"
#include "pipelinestruct.h"

/* InputFilter:
 *
 * Initializes parallel encoders and pipeline units. Counts blocks and
 * reads in one at a time to PipelineUnit.rawBuffer.
 *
 * The number of tokens in the pipeline is set here (num_tokens).
 *
 * This filter is serial, due to I/O and being first filter.
 */
class InputFilter: public tbb::filter {
public:
    static const size_t num_tokens = 6;
    InputFilter( FILE* fin, unsigned s, size_t tot, FLAC::Encoder::File *encoder, unsigned int compression_levels);
    ~InputFilter();
    int count;
private:
    FILE* input_file;
    unsigned size;
    size_t need;
    size_t left;
    size_t next_unit;
    FLAC::Encoder::File *encoder; //main pointer to the shared encoder (each unit is given this ptr)
    PipelineUnit unit[num_tokens];
    void* operator()(void*);
};

/* PCMFilter:
 *
 * A simple filter which converts the packed little-endian 16-bit PCM samples
 * in PipelineUnit.rawBuffer from WAVE into an interleaved FLAC__int32 buffer
 * for libFLAC. 
 *
 * This filter is parallel. 
 */
class PCMFilter: public tbb::filter {
public:
    PCMFilter();
    void* operator()( void* item );
};

/* ProcessFilter:
 *
 * This filter calls the process_interleaved_no_write function defined in libFLAC.
 * That function processes the buffer according to the FLAC specification,
 * but does not write out to disk. After calling process_interleaved_no_write,
 * PipelineUnit.localEncoder has a frame buffer ready to write out.
 *
 * See the comments in libFLAC for a deeper understanding of how the function changed
 * from its original serial version.
 *
 * This filter is parallel.
 */
class ProcessFilter: public tbb::filter {
public:
    ProcessFilter();
    void* operator()( void* item );
};

/* OutputFilter:
 *
 * This filter keeps track of the current frame number of the audio file. It first
 * tells the local encoder its frame number before passing it along to write_parallel.
 * The block is written only if it is not the last block - that block must be handled
 * by the shared encoder outside of the pipeline.
 *
 * This filter is serial, due to I/O.
 */
class OutputFilter: public tbb::filter {
public:
    OutputFilter();
    void* operator()( void* item );
private:
    unsigned current_frame_number;
};
