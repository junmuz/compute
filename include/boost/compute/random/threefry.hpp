// Added By: Muhammad Junaid Muzammil <mjunaidmuzammil@gmail.com>
// Copyright 2010-2012, D. E. Shaw Research.
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:

// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions, and the following disclaimer.

// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions, and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.

// * Neither the name of D. E. Shaw Research nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


#ifndef BOOST_COMPUTE_RANDOM_THREEFRY_HPP
#define BOOST_COMPUTE_RANDOM_THREEFRY_HPP

#include <boost/compute/types.hpp>
#include <boost/compute/buffer.hpp>
#include <boost/compute/kernel.hpp>
#include <boost/compute/context.hpp>
#include <boost/compute/program.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/algorithm/transform.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>
#include <boost/compute/detail/program_cache.hpp>
#include <boost/compute/iterator/discard_iterator.hpp>

namespace boost {
namespace compute {
namespace random {


/// \class threefry_rng_engine
/// \brief Threefry pseudorandom number generator.
template<class T>
class threefry_engine
{
public:
    typedef T result_type;

    /// Creates a new threefry_engine and seeds it with \p value.
    explicit threefry_engine(command_queue &queue)
        : m_context(queue.get_context())
    {
        // setup program
        load_program();
    }

    /// Creates a new threefry_engine object as a copy of \p other.
    threefry_engine(const threefry_engine<T> &other)
        : m_context(other.m_context)
    {
    }

    /// Copies \p other to \c *this.
    threefry_engine<T>& operator=(const threefry_engine<T> &other)
    {
        if(this != &other){
            m_context = other.m_context;
        }

        return *this;
    }

    /// Destroys the threefry_engine object.
    ~threefry_engine()
    {
    }

private:
    /// \internal_
    void load_program()
    {
        boost::shared_ptr<detail::program_cache> cache =
            detail::get_program_cache(m_context);
        std::string cache_key =
            std::string("threefry_engine_32x2");

        m_program = cache->get(cache_key);
        if(!m_program.get()){
            const char source[] =
                "#define THREEFRY2x32_DEFAULT_ROUNDS 20\n"
                "#define SKEIN_KS_PARITY_32 0x1BD11BDA\n"

		"enum r123_enum_threefry32x2 {\n"
                "    R_32x2_0_0=13,\n"
                "    R_32x2_1_0=15,\n"
                "    R_32x2_2_0=26,\n"
                "    R_32x2_3_0= 6,\n"
                "    R_32x2_4_0=17,\n"
                "    R_32x2_5_0=29,\n"
                "    R_32x2_6_0=16,\n"
                "    R_32x2_7_0=24\n"
                "}\n"

                "static inline uint32_t RotL_32(uint32_t x, unsigned int N)\n"
                "{\n"
                "    return (x << (N & 31)) | (x >> ((32-N) & 31));\n"
                "}\n"
                
                "struct r123array2x32 {\n"
                "    uint32_t *v;\n"
                "}\n"
                "typedef struct r123array2x32 threefry2x32_ctr_t;\n"
                "typedef struct r123array2x32 threefry2x32_key_t;\n"

                "threefry2x32_ctr_t threefry2x32_R(unsigned int Nrounds, threefry2x32_ctr_t in, threefry2x32_key_t k))\n"
                "{\n"
                "    threefry2x32_ctr_t X;\n"
                "    uint32_t ks[3];\n"
                "    int  i; /* avoid size_t to avoid need for stddef.h */\n"
                "    assert(Nrounds<=32);\n"
                "    ks[2] =  SKEIN_KS_PARITY_32;\n"
                "    for (i=0;i < 2; i++) {\n"
                "        ks[i] = k.v[i];\n"
                "        X.v[i]  = in.v[i];\n"
                "        ks[2] ^= k.v[i];\n"
                "    }\n"
                "    X.v[0] += ks[0]; X.v[1] += ks[1];\n"
                "    if(Nrounds>0){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_0_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>1){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_1_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>2){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_2_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>3){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_3_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>3){\n"
                "        /* InjectKey(r=1) */\n"
                "        X.v[0] += ks[1]; X.v[1] += ks[2];\n"
                "        X.v[1] += 1;     /* X.v[2-1] += r  */\n"
                "    }\n"
                "    if(Nrounds>4){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_4_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>5){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_5_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>6){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_6_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>7){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_7_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>7){\n"
                "        /* InjectKey(r=2) */\n"
                "        X.v[0] += ks[2]; X.v[1] += ks[0];\n"
                "        X.v[1] += 2;\n"
                "    }\n"
                "    if(Nrounds>8){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_0_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>9){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_1_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>10){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_2_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>11){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_3_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>11){\n"
                "        /* InjectKey(r=3) */\n"
                "        X.v[0] += ks[0]; X.v[1] += ks[1];\n"
                "        X.v[1] += 3;\n"
                "    }\n"
                "    if(Nrounds>12){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_4_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>13){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_5_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>14){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_6_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>15){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_7_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>15){\n"
                "        /* InjectKey(r=4) */\n"
                "        X.v[0] += ks[1]; X.v[1] += ks[2];\n"
                "        X.v[1] += 4;\n"
                "    }\n"
                "    if(Nrounds>16){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_0_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>17){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_1_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>18){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_2_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>19){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_3_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>19){\n"
                "        /* InjectKey(r=5) */\n"
                "        X.v[0] += ks[2]; X.v[1] += ks[0];\n"
                "        X.v[1] += 5;\n"
                "    }\n"
                "    if(Nrounds>20){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_4_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>21){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_5_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>22){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_6_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>23){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_7_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>23){\n"
                "        /* InjectKey(r=6) */\n"
                "        X.v[0] += ks[0]; X.v[1] += ks[1];\n"
                "        X.v[1] += 6;\n"
                "    }\n"
                "    if(Nrounds>24){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_0_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>25){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_1_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>26){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_2_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>27){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_3_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>27){\n"
                "        /* InjectKey(r=7) */\n"
                "        X.v[0] += ks[1]; X.v[1] += ks[2];\n"
                "        X.v[1] += 7;\n"
                "    }\n"
                "    if(Nrounds>28){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_4_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>29){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_5_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>30){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_6_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>31){  X.v[0] += X.v[1]; X.v[1] = RotL_32(X.v[1],R_32x2_7_0); X.v[1] ^= X.v[0]; }\n"
                "    if(Nrounds>31){\n"
                "    /* InjectKey(r=8) */\n"
                "        X.v[0] += ks[2]; X.v[1] += ks[0];\n"
                "        X.v[1] += 8;\n"
                "    }\n"
                "    return X;\n"
                "}\n";

                "__kernel void generate_rng(uint32_t *ctr, uint32_t *key) {\n"
                "    threefry2x32_ctr_t in;\n"
                "    in.v = ctr;\n"
                "    threefry2x32_key_t k;\n"
                "    k.v = key;\n"
                "    threefry2x32_R(20, in, k);\n"
                "}\n"

            m_program = program::build_with_source(source, m_context);

            cache->insert(cache_key, m_program);
        }
    }

    void generate(command_queue &queue, uint32_t *ctr_val, uint32_t *key_val) {
        kernel rng_kernel = m_program.create_kernel("generate_rng");
        rng_kernel.set_arg(0, ctr_val);
        rng_kernel.set_arg(1, key_val);
        queue.enqueue_1d_range_kernel(rng_kernel, , 1, 0);
    }

private:
    context m_context;
    program m_program;
};

typedef threefry_engine<uint_> threefry;

} // end random namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_RANDOM_THREEFRY_HPP