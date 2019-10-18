//
// Created by wuyuncheng on 18/10/19.
//

#ifndef COLLABORATIVEML_PB_CONVERTER_H
#define COLLABORATIVEML_PB_CONVERTER_H

#include "encoder.h"

/**
 * pb serialize batch ids
 *
 * @param batch_ids
 * @param size
 * @param output_str
 */
void serialize_batch_ids(int *batch_ids, int size, std::string & output_str);

/**
 * pb deserialize batch ids
 *
 * @param batch_ids
 * @param input_str
 */
void deserialize_ids_from_string(int *& batch_ids, std::string input_str);

/**
 * pb serialize encoded number
 *
 * @param number
 * @param output_str
 */
void serialize_encoded_number(EncodedNumber number, std::string & output_str);

/**
 * pb deserialize encoded number
 *
 * @param number
 * @param input_str
 */
void deserialize_number_from_string(EncodedNumber & number, std::string input_str);

/**
 * pb serialize partial sums
 *
 * @param partial_sums
 * @param size
 * @param output_str
 */
void serialize_partial_sums(EncodedNumber *partial_sums, int size, std::string & output_str);

/**
 * pb deserialize partial sums
 *
 * @param partial_sums
 * @param input_str
 */
void deserialize_sums_from_string(EncodedNumber *& partial_sums, std::string input_str);

/**
 * pb serialize batch losses
 *
 * @param batch_losses
 * @param size
 * @param output_str
 */
void serialize_batch_losses(EncodedNumber *batch_losses, int size, std::string & output_str);

/**
 * pb deserialize batch losses
 *
 * @param batch_losses
 * @param input_str
 */
void deserialize_losses_from_string(EncodedNumber *& batch_losses, std::string input_str);

#endif //COLLABORATIVEML_PB_CONVERTER_H
