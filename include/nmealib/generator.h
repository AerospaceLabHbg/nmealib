/*
 * This file is part of nmealib.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __NMEALIB_GENERATOR_H__
#define __NMEALIB_GENERATOR_H__

#include <nmealib/info.h>

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

/* forward declaration */
typedef struct _NmeaGenerator NmeaGenerator;

/**
 * Generator type enum
 */
typedef enum _NmeaGeneratorType {
  NMEALIB_GENERATOR_NOISE        = 0u,
  NMEALIB_GENERATOR_STATIC       = 1u,
  NMEALIB_GENERATOR_ROTATE       = 2u,
  NMEALIB_GENERATOR_SAT_STATIC   = 3u,
  NMEALIB_GENERATOR_SAT_ROTATE   = 4u,
  NMEALIB_GENERATOR_POS_RANDMOVE = 5u,
  NMEALIB_GENERATOR_LAST         = NMEALIB_GENERATOR_POS_RANDMOVE
} NmeaGeneratorType;

/**
 * Generator initialiser function definition.
 *
 * @param gen a pointer to the generator
 * @param info a pointer to an nmeaINFO structure to use during generation
 * @return true on success, false otherwise
 */
typedef bool (*NmeaGeneratorInit)(NmeaGenerator *gen, NmeaInfo *info);

/**
 * Generator loop function definition.
 *
 * @param gen a pointer to the generator
 * @param info a pointer to an nmeaINFO structure to use during generation
 * @return true on success, false otherwise
 */
typedef bool (*NmeaGeneratorLoop)(NmeaGenerator *gen, NmeaInfo *info);

/**
 * Generator reset function definition.
 *
 * @param gen a pointer to the generator
 * @param info a pointer to an nmeaINFO structure to use during generation
 * @return true on success, false otherwise
 */
typedef bool (*NmeaGeneratorReset)(NmeaGenerator *gen, NmeaInfo *info);

/**
 * Generator destroy function definition.
 *
 * @param gen a pointer to the generator
 * @return true on success, false otherwise
 */
typedef bool (*NmeaGeneratorDestroy)(NmeaGenerator *gen);

/**
 * Generator structure
 */
typedef struct _NmeaGenerator {
    void                 *gen_data;     /**< generator data       */
    NmeaGeneratorInit     init_call;    /**< initialiser function */
    NmeaGeneratorLoop     loop_call;    /**< loop function        */
    NmeaGeneratorReset    reset_call;   /**< reset function       */
    NmeaGeneratorDestroy  destroy_call; /**< destroy function     */
    NmeaGenerator        *next;         /**< the next generator   */
} NmeaGenerator;

/**
 * Initialise the generator
 *
 * @param gen The generator
 * @param info An info structure to use during generation (present and smask
 * are preserved, other fields are reset before generation starts)
 * @return True on success
 */
bool nmeaGeneratorInit(NmeaGenerator *gen, NmeaInfo *info);

/**
 * Loop the generator.
 *
 * @param gen The generator
 * @param info An info structure to use during generation
 * @return True on success
 */
bool nmeaGeneratorLoop(NmeaGenerator *gen, NmeaInfo *info);

/**
 * Reset the generator.
 *
 * @param gen The generator
 * @param info An info structure to use during generation
 * @return True on success
 */
bool nmeaGeneratorReset(NmeaGenerator *gen, NmeaInfo *info);

/**
 * Destroy the generator.
 *
 * @param gen The generator
 */
void nmeaGeneratorDestroy(NmeaGenerator *gen);

/**
 * Add a generator to the existing ones.
 *
 * @param to The generators to add to
 * @param gen The generator to add
 */
void nmeaGeneratorAdd(NmeaGenerator *to, NmeaGenerator *gen);

/**
 * Run a new generation loop on the generator
 *
 * @param buf The string buffer in which to generate
 * @param info An info structure to use during generation
 * @param gen The generator
 * @param generate_mask The mask (smask) of sentences to generate
 * @return The total length of the generated sentences
 */
size_t nmeaGeneratorGenerateFrom(char **buf, NmeaInfo *info, NmeaGenerator *gen, int generate_mask);

/**
 * Create the generator and initialise it.
 *
 * @param type The type of the generator to create (see nmeaGENTYPE)
 * @param info An info structure to use during generation
 * @return The generator
 */
NmeaGenerator * nmeaGeneratorCreate(NmeaGeneratorType type, NmeaInfo *info);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __NMEALIB_GENERATOR_H__ */
