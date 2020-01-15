from utils import load_from_csv
from utils import AverageMeter, eval_criterion

from sklearn.tree import DecisionTreeClassifier, DecisionTreeRegressor
from sklearn.ensemble import RandomForestClassifier, GradientBoostingClassifier
from sklearn.ensemble import RandomForestRegressor, GradientBoostingRegressor

# Global hyper-parameters
'''
NUM_EXPS:       number of experiment repeats, report mean value
DATASET:        the name of the dataset for evaluation
DECISION_TREE:  flags whether to train with decision tree
RANDOM_FOREST:  flags whether to train with Random Forest
GBDT:           flags whether to train with GBDT
'''
NUM_EXPS = 10
DECISION_TREE = False
RANDOM_FOREST = False
GBDT = True

# loading dataset
DATA_DIR = 'utils/data/'
DATASET = 'air_quality.data'
X_train, y_train, X_test, y_test = load_from_csv(DATA_DIR+DATASET)
IS_CLASSIFICATION = False
if not IS_CLASSIFICATION:
    EVAL_METRIC = 'rmse'             # evaluation metric for regression

print(f'Data shape: \ntrain\t x {X_train.shape},\ty {y_train.shape}\n'
      f'test\t x {X_test.shape},\ty {y_test.shape}\n')


train_avg, test_avg = AverageMeter(), AverageMeter()
###########################         Decision Tree hyper-parameters          ###########################
# shared hyper-parameters
MAX_DEPTH = 8
MIN_SAMPLES_SPLIT = 2
MIN_SAMPLES_LEAF = 1
MIN_IMPURITY_DECREASE = 1e-5

# hyper-parameters for classification
if IS_CLASSIFICATION: CRITERION = 'gini'        # 'gini' or 'entropy'
else: CRITERION = 'mse'                         # “mse”, “friedman_mse”, “mae”

if DECISION_TREE:
    train_avg.reset(); test_avg.reset()
    for _ in range(NUM_EXPS):
        if IS_CLASSIFICATION:
            clf = DecisionTreeClassifier(criterion=CRITERION, max_depth=MAX_DEPTH,
                    min_samples_split=MIN_SAMPLES_SPLIT, min_samples_leaf=MIN_SAMPLES_LEAF,
                    min_impurity_decrease=MIN_IMPURITY_DECREASE)
        else:
            clf = DecisionTreeRegressor(criterion=CRITERION, max_depth=MAX_DEPTH,
                    min_samples_split=MIN_SAMPLES_SPLIT, min_samples_leaf=MIN_SAMPLES_LEAF,
                    min_impurity_decrease=MIN_IMPURITY_DECREASE)

        clf.fit(X_train, y_train)

        if IS_CLASSIFICATION:
            train_perf = clf.score(X_train, y_train)
            test_perf = clf.score(X_test, y_test)
        else:
            train_perf = eval_criterion(y_train, clf.predict(X_train), metric=EVAL_METRIC)
            test_perf = eval_criterion(y_test, clf.predict(X_test), metric=EVAL_METRIC)

        train_avg.update(train_perf)
        test_avg.update(test_perf)

    print(f'Decision Tree\t {NUM_EXPS}-Avg train_perf:\t{train_avg.avg:4f}, test_perf:\t{test_avg.avg:4f}')

###########################         Random Forest hyper-parameters          ###########################
NUM_TREE = 50
MAX_DEPTH = 8
MIN_SAMPLES_SPLIT = 2
MIN_SAMPLES_LEAF = 1
MIN_IMPURITY_DECREASE = 1e-5

# hyper-parameters for classification
if IS_CLASSIFICATION:
    CRITERION = 'gini'
else:
    CRITERION = 'mse'

if RANDOM_FOREST:
    train_avg.reset(); test_avg.reset()
    for _ in range(NUM_EXPS):
        if IS_CLASSIFICATION:
            clf = RandomForestClassifier(n_estimators=NUM_TREE, criterion=CRITERION, max_depth=MAX_DEPTH,
                    min_samples_split=MIN_SAMPLES_SPLIT, min_samples_leaf=MIN_SAMPLES_LEAF,
                    min_impurity_decrease=MIN_IMPURITY_DECREASE)
        else:
            clf = RandomForestRegressor(n_estimators=NUM_TREE, criterion=CRITERION, max_depth=MAX_DEPTH,
                    min_samples_split=MIN_SAMPLES_SPLIT, min_samples_leaf=MIN_SAMPLES_LEAF,
                    min_impurity_decrease=MIN_IMPURITY_DECREASE)

        clf.fit(X_train, y_train)

        if IS_CLASSIFICATION:
            train_perf = clf.score(X_train, y_train)
            test_perf = clf.score(X_test, y_test)
        else:
            train_perf = eval_criterion(y_train, clf.predict(X_train), metric=EVAL_METRIC)
            test_perf = eval_criterion(y_test, clf.predict(X_test), metric=EVAL_METRIC)

        train_avg.update(train_perf)
        test_avg.update(test_perf)

    print(f'Random Forest\t {NUM_EXPS}-Avg train_perf:\t{train_avg.avg:4f}, test_perf:\t{test_avg.avg:4f}')


###########################            GDBT hyper-parameters              ###########################
LEARNING_RATE = 0.1
N_ESTIMATORS = 100              # number of boosting stages to perform
SUBSAMPLE = 0.8
CRITERION = 'friedman_mse'      # 'friedman_mse' or 'mse
MIN_SAMPLES_SPLIT = 2
MIN_SAMPLES_LEAF = 1
MAX_DEPTH = 8
MIN_IMPURITY_DECREASE = 1e-5
VALIDATION_FRACTION = 0.1       # proportion of training for early stopping
VERBOSE = True                  # verbose output

if IS_CLASSIFICATION:
    LOSS = 'deviance'           # (= logistic regression); 'exponential' for AdaBoost
else:
    LOSS = 'ls'

if GBDT:
    train_avg.reset(); test_avg.reset()
    for _ in range(NUM_EXPS):
        if IS_CLASSIFICATION:
            clf = GradientBoostingClassifier(loss=LOSS, learning_rate=LEARNING_RATE,  n_estimators=N_ESTIMATORS,
                    subsample=SUBSAMPLE, criterion=CRITERION, min_samples_split=MIN_SAMPLES_SPLIT, max_depth=MAX_DEPTH,
                    min_samples_leaf=MIN_SAMPLES_LEAF, min_impurity_decrease=MIN_IMPURITY_DECREASE,
                    validation_fraction=VALIDATION_FRACTION, verbose=True)
        else:
            clf = GradientBoostingRegressor(loss=LOSS, learning_rate=LEARNING_RATE, n_estimators=N_ESTIMATORS,
                     subsample=SUBSAMPLE, criterion=CRITERION,
                     min_samples_split=MIN_SAMPLES_SPLIT, max_depth=MAX_DEPTH,
                     min_samples_leaf=MIN_SAMPLES_LEAF,
                     min_impurity_decrease=MIN_IMPURITY_DECREASE,
                     validation_fraction=VALIDATION_FRACTION, verbose=True)

        clf.fit(X_train, y_train)

        if IS_CLASSIFICATION:
            train_perf = clf.score(X_train, y_train)
            test_perf = clf.score(X_test, y_test)
        else:
            train_perf = eval_criterion(y_train, clf.predict(X_train), metric=EVAL_METRIC)
            test_perf = eval_criterion(y_test, clf.predict(X_test), metric=EVAL_METRIC)

        train_avg.update(train_perf)
        test_avg.update(test_perf)

    print(f'GBDT\t {NUM_EXPS}-Avg train_perf:\t{train_avg.avg:4f}, test_perf:\t{test_avg.avg:4f}')
