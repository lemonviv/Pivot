import numpy as np
from sklearn.datasets import make_regression, make_classification

'''
Regression doc:     https://scikit-learn.org/stable/modules/generated/sklearn.datasets.make_regression.html
Classification doc: https://scikit-learn.org/stable/modules/generated/sklearn.datasets.make_classification.html
'''

# Global shared hyper-parameters
IS_REGRESSION = False
NUM_SAMPLES = 100
NUM_FEATURES = 10
NUM_INFORMATIVE_FEATS = 5
NUM_TARGETS = 2             # 1 for regression, number of classes for Classification
ROOT_DIR = 'data/'
SAVE_PATH = ROOT_DIR+f'{"reg" if IS_REGRESSION else "cls"}_{NUM_SAMPLES}_{NUM_FEATURES}_' \
            f'{NUM_INFORMATIVE_FEATS}_{NUM_TARGETS}.csv'
RANDOM_SEED = 2020

# Regression hyper-parameters
BIAS = 0.
NOISE = 1.
EFFECTIVE_RANK = None       # default value recommended
TAIL_STRENGTH = 2.5         # default value recommended
COEF = False

# Classification hyper-parameters
NUM_REDUNDANT = 2           # redundant features generated as random linear combinations of the informative features
NUM_CLUSTERS_PER_CLASS = 2  # number of clusters per class
RANDOM_LABEL_PERC = 0.01    # fraction of samples whose class is assigned randomly
CLASS_SEP = 1.              # factor multiplying the hypercube size, default value recommended
HYPERCUBE = True            # default value recommended



if IS_REGRESSION:
    # regression data generation
    # X: N*M; y: N; if COEF==True: COEF: N
    assert NUM_TARGETS == 1
    X, y, coef_ = make_regression(n_samples=NUM_SAMPLES, n_features=NUM_FEATURES, n_informative=NUM_INFORMATIVE_FEATS,
                      n_targets=NUM_TARGETS, bias=BIAS, effective_rank=EFFECTIVE_RANK, tail_strength=TAIL_STRENGTH,
                      noise=NOISE, coef=True, random_state=RANDOM_SEED)

    # reshape y to [N, 1] and pack [X, y] into data
    y.resize(NUM_SAMPLES, 1)
    data_ = np.concatenate([X, y], axis=-1)

    # concatenate coef to the first line
    coef = np.concatenate([coef_, [0.]*NUM_TARGETS], axis=0).reshape(1, NUM_FEATURES+NUM_TARGETS)
    data = np.concatenate([coef, data_], axis=0)
    if not COEF: data = data[1:]

else:
    # classification data generation
    # X: N*M; y: N
    assert NUM_TARGETS >= 2
    X, y  = make_classification(n_samples=NUM_SAMPLES, n_features=NUM_FEATURES, n_informative=NUM_INFORMATIVE_FEATS,
                n_redundant=NUM_REDUNDANT, n_classes=NUM_TARGETS, n_clusters_per_class=NUM_CLUSTERS_PER_CLASS,
                flip_y=RANDOM_LABEL_PERC, class_sep=CLASS_SEP, hypercube=HYPERCUBE, random_state=RANDOM_SEED)

    # reshape y to [N, 1] and pack [X, y] into data
    y.resize(NUM_SAMPLES, 1)
    data = np.concatenate([X, y], axis=-1)

# save data to csv
np.savetxt(SAVE_PATH, data, fmt='%.6f', delimiter=',')
