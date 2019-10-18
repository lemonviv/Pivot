//
// Created by wuyuncheng on 16/10/19.
//

#include "test_logistic_regression.h"
#include "../src/utils/djcs_t_aux.h"


extern hcs_random *hr;
extern djcs_t_public_key *pk;
extern djcs_t_private_key *vk;
//djcs_t_auth_server **au2 = (djcs_t_auth_server **)malloc(TOTAL_CLIENT_NUM * sizeof(djcs_t_auth_server *));
//static mpz_t *si2 = (mpz_t *)malloc(TOTAL_CLIENT_NUM * sizeof(mpz_t));
extern djcs_t_auth_server **au;
extern mpz_t *si;
extern mpz_t n, positive_threshold, negative_threshold;
extern int total_cases_num, passed_cases_num;
LogisticRegression lr_model(2, 10, PRECISION_THRESHOLD, 3);



void compute_thresholds(djcs_t_public_key *pk) {
    mpz_t g;
    mpz_init(g);
    mpz_set(g, pk->g);
    mpz_sub_ui(n, g, 1);

    mpz_t t;
    mpz_init(t);
    mpz_fdiv_q_ui(t, n, 3);
    mpz_sub_ui(positive_threshold, t, 1);  // this is positive threshold
    mpz_sub(negative_threshold, n, positive_threshold);  // this is negative threshold

    mpz_clear(g);
    mpz_clear(t);
}


void test_init_weights(djcs_t_public_key *pk, hcs_random *hr) {

    logger(stdout, "Test init weights\n");

    float x;
    lr_model.init_encrypted_local_weights(pk, hr);
    for (int i = 0; i < lr_model.feature_num; i++) {
        EncodedNumber tmp;
        decrypt_temp(pk, au, TOTAL_CLIENT_NUM, tmp, lr_model.local_weights[i]);
        tmp.decode(x);
        logger(stdout, "The decrypted local weight %d = %f\n", i, x);
    }
}


void test_instance_partial_sum(djcs_t_public_key *pk, hcs_random *hr, int feature_num) {

    logger(stdout, "Test instance partial sum\n");

    lr_model.init_encrypted_local_weights(pk, hr);

    EncodedNumber *instance = new EncodedNumber[feature_num];
    for (int i = 0; i < feature_num; i++) {
        instance[i].set_float(n, (i+1) * 0.05, FLOAT_PRECISION);
        logger(stdout, "The feature %d = %f\n", i, (i+1) * 0.05);
    }

    for (int i = 0; i < feature_num; i++) {
        EncodedNumber t;
        decrypt_temp(pk, au, TOTAL_CLIENT_NUM, t, lr_model.local_weights[i]);
        float x;
        t.decode(x);
        logger(stdout, "decoded local weight %d = %f\n", i, x);
    }

    EncodedNumber res;
    float x;

    lr_model.instance_partial_sum(pk, hr, instance, res);
    decrypt_temp(pk, au, TOTAL_CLIENT_NUM, res, res);
    res.decode_with_truncation(x, 0 - FLOAT_PRECISION);
    logger(stdout, "The instance partial sum is %f\n", x);
}


void test_partial_predict(djcs_t_public_key *pk, hcs_random *hr, int feature_num) {

    logger(stdout, "Test partial predict\n");

    lr_model.init_encrypted_local_weights(pk, hr);

    EncodedNumber *instance = new EncodedNumber[feature_num];
    for (int i = 0; i < feature_num; i++) {
        instance[i].set_float(n, -(i+1) * 0.05, FLOAT_PRECISION);
        logger(stdout, "The feature %d = %f\n", i, -(i+1) * 0.05);
    }

    EncodedNumber res;
    float x;

    lr_model.partial_predict(pk, hr, instance, res);
    decrypt_temp(pk, au, TOTAL_CLIENT_NUM, res, res);
    res.decode_with_truncation(x, 0 - FLOAT_PRECISION);
    logger(stdout, "The partial predict is %f\n", x);
}


void test_aggregate_partial_sums(djcs_t_public_key *pk, hcs_random *hr, int client_num) {

    EncodedNumber res;
    float x;
    float sum = 0.0;
    srand(static_cast<unsigned> (time(0)));

    EncodedNumber *partial_plains = new EncodedNumber[client_num];
    EncodedNumber *partial_ciphers = new EncodedNumber[client_num];
    for (int i = 0; i < client_num; i++) {
        float partial = static_cast<float>(rand()) / static_cast<float> (RAND_MAX);
        //logger(stdout, "the partial plains %d = %f\n", i, partial);
        partial_plains[i].set_float(n, partial, FLOAT_PRECISION);
        djcs_t_aux_encrypt(pk, hr, partial_ciphers[i], partial_plains[i]);
        sum += partial;
    }

    sum = 1.0 / (1 + exp(0 - sum));

    // homomorphic addition to aggregated_sum
//    res.set_float(n2, 0.0);
//    djcs_t_aux_encrypt(pk, hr, res, res);
//    for (int i = 0; i < client_num; ++i) {
//        EncodedNumber t;
//        djcs_t_aux_ee_add(pk, res, res, partial_ciphers[i]);
//        decrypt_temp(pk, au2, TOTAL_CLIENT_NUM, t, res);
//        float y;
//        t.decode(y);
//        logger(stdout, "add cipher %i, get result = %f\n", i, y);
//    }

    lr_model.aggregate_partial_sum_instance(pk, hr, partial_ciphers, res, client_num);

    decrypt_temp(pk, au, TOTAL_CLIENT_NUM, res, res);
    res.decode_with_truncation(x, 0 - FLOAT_PRECISION);

    if (fabs(sum - x) >= PRECISION_THRESHOLD) {
        logger(stdout, "test_aggregated_partial_sum: "
                       "the aggregated sum %f is not match plaintext sum %f, failed\n", x, sum);
        total_cases_num += 1;
    } else {
        logger(stdout, "test_aggregated_partial_sum: "
                       "succeed, the aggregated sum %f is equal to plaintext sum %f\n", x, sum);
        total_cases_num += 1;
        passed_cases_num += 1;
    }
}


void test_compute_batch_losses(djcs_t_public_key *pk, hcs_random *hr, int batch_size) {

    float x;
    srand(static_cast<unsigned> (time(0)));

    EncodedNumber *losses = new EncodedNumber[batch_size];
    EncodedNumber *labels = new EncodedNumber[batch_size];
    EncodedNumber *aggregate_sums = new EncodedNumber[batch_size];
    for (int i = 0; i < batch_size; i++) {
        float r = static_cast<float>(rand()) / static_cast<float> (RAND_MAX);
        //logger(stdout, "the partial plains %d = %f\n", i, partial);
        aggregate_sums[i].set_float(n, r, FLOAT_PRECISION);
        djcs_t_aux_encrypt(pk, hr, aggregate_sums[i], aggregate_sums[i]);

        int x = i % 2;
        float label = pow(-1, x);
        label = label > 0 ? 0 : label;
        labels[i].set_float(n, label);
        //logger(stdout, "r %d = %f\n", i, r);
        //logger(stdout, "label %d = %f\n", i, label);
        logger(stdout, "loss %d = %f\n", i, label + r);
    }

    lr_model.compute_batch_loss(pk, hr, aggregate_sums, labels, losses);

    for (int i = 0; i < batch_size; i++) {
        decrypt_temp(pk, au, TOTAL_CLIENT_NUM, losses[i], losses[i]);
        losses[i].decode(x);
        logger(stdout, "decrypted and decoded loss %d = %f\n", i, x);
    }
}


void test_update_weights(djcs_t_public_key *pk, hcs_random *hr, int feature_num, int batch_size, float alpha) {

    // simulate batch_data with batch_size rows and feature_num columns
    float x;
    srand(static_cast<unsigned> (time(0)));

    lr_model.init_encrypted_local_weights(pk, hr);

    EncodedNumber *losses = new EncodedNumber[batch_size];
    EncodedNumber **batch_data = new EncodedNumber*[batch_size];
    for (int i = 0; i < batch_size; i++) {
        batch_data[i] = new EncodedNumber[feature_num];
    }

    for (int i = 0; i < batch_size; i++) {
        float r = static_cast<float>(rand()) / static_cast<float> (RAND_MAX);
        logger(stdout, "the losses %d = %f\n", i, r);
        losses[i].set_float(n, r, FLOAT_PRECISION);
        djcs_t_aux_encrypt(pk, hr, losses[i], losses[i]);
    }

    for (int i = 0; i < batch_size; i++) {
        for (int j = 0; j < feature_num; j++) {
            float r = static_cast<float>(rand()) / static_cast<float> (RAND_MAX);
            logger(stdout, "the batch[%d][%d] = %f\n", i, j, r);
            batch_data[i][j].set_float(n, r, FLOAT_PRECISION);
            //djcs_t_aux_encrypt(pk, hr, batch_data[i][j], batch_data[i][j]);
        }
    }

    lr_model.update_local_weights(pk, hr, batch_data, losses, alpha);

    for (int i = 0; i < feature_num; i++) {
        EncodedNumber t;
        decrypt_temp(pk, au, TOTAL_CLIENT_NUM, t, lr_model.local_weights[i]);
        t.decode(x);
        logger(stdout, "updated weight %d = %f\n", i, x);
    }
}


int test_lr() {

    logger(stdout, "****** Test logistic regression functions ******\n");

//    hcs_random *hr = hcs_init_random();
//    djcs_t_public_key *pk = djcs_t_init_public_key();
//    djcs_t_private_key *vk = djcs_t_init_private_key();
//    djcs_t_generate_key_pair(pk, vk, hr, 1, 1024, 3, 3);
//
//    mpz_t *coeff = djcs_t_init_polynomial(vk, hr);
//    for (int i = 0; i < TOTAL_CLIENT_NUM; i++) {
//        mpz_init(si[i]);
//        djcs_t_compute_polynomial(vk, coeff, si[i], i);
//        au[i] = djcs_t_init_auth_server();
//        djcs_t_set_auth_server(au[i], si[i], i);
//    }
//
//    mpz_init(n);
//    mpz_init(positive_threshold);
//    mpz_init(negative_threshold);

    total_cases_num = 0;
    passed_cases_num = 0;

    // compute_thresholds(pk);

    // test init local weights
    test_init_weights(pk, hr);

    // test instance partial sum
    test_instance_partial_sum(pk, hr, 3);

    // test partial predict
    test_partial_predict(pk, hr, 3);

    // test aggregated partial sum
    test_aggregate_partial_sums(pk, hr, 3);

    // test compute batch loss
    test_compute_batch_losses(pk, hr, 2);

    // test update weight
    test_update_weights(pk, hr, 3, 2, 1.0);

    logger(stdout, "****** total_cases_num = %d, passed_cases_num = %d ******\n",
           total_cases_num, passed_cases_num);

    // free memory
//    hcs_free_random(hr);
//    djcs_t_free_public_key(pk);
//    djcs_t_free_private_key(vk);
//
//    mpz_clear(n);
//    mpz_clear(positive_threshold);
//    mpz_clear(negative_threshold);
//
//    for (int i = 0; i < TOTAL_CLIENT_NUM; i++) {
//        mpz_clear(si[i]);
//        djcs_t_free_auth_server(au[i]);
//    }
//    free(si);
//    free(au);
//
//    djcs_t_free_polynomial(vk, coeff);

    return 0;
}