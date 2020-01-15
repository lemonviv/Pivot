from utils import load_CreditCard_dataset, load_Credit_dataset
from utils import AverageMeter

from sklearn import tree
from sklearn.ensemble import RandomForestClassifier, GradientBoostingClassifier

# Global hyper-parameters
'''
NUM_EXPS:       number of experiment repeats, report mean value
DATASET:        the name of the dataset for evaluation
DECISION_TREE:  flags whether to train with decision tree
RANDOM_FOREST:  flags whether to train with Random Forest
GBDT:           flags whether to train with GBDT
'''
NUM_EXPS = 10
DECISION_TREE = True
RANDOM_FOREST = False
GBDT = False

# loading dataset
DATASET = 'creditcard'
if DATASET == 'credit':
    X_train, y_train, X_test, y_test = load_Credit_dataset()
elif DATASET == 'creditcard':
    X_train, y_train, X_test, y_test = load_CreditCard_dataset()
else:
    raise Exception(f'can not find dataset {DATASET}')

print(f'Data shape: \ntrain\t x {X_train.shape},\ty {y_train.shape}\n'
      f'test\t x {X_test.shape},\ty {y_test.shape}\n')


train_avg, test_avg = AverageMeter(), AverageMeter()
###########################         Decision Tree hyper-parameters          ###########################
CRITERION = 'gini'                      # 'gini' or 'entropy'
MAX_DEPTH = 8
MIN_SAMPLES_SPLIT = 2
MIN_SAMPLES_LEAF = 1
MIN_IMPURITY_DECREASE = 1e-5

if DECISION_TREE:
    train_avg.reset(); test_avg.reset()
    for _ in range(NUM_EXPS):
        clf = tree.DecisionTreeClassifier(criterion=CRITERION, max_depth=MAX_DEPTH,
                    min_samples_split=MIN_SAMPLES_SPLIT, min_samples_leaf=MIN_SAMPLES_LEAF,
                    min_impurity_decrease=MIN_IMPURITY_DECREASE)
        clf.fit(X_train, y_train)
        train_acc = clf.score(X_train, y_train)
        test_acc = clf.score(X_test, y_test)

        train_avg.update(train_acc)
        test_avg.update(test_acc)

    print(f'Decision Tree\t {NUM_EXPS}-Avg train_acc:\t{train_avg.avg:4f}, test_acc:\t{test_avg.avg:4f}')

###########################         Random Forest hyper-parameters          ###########################
NUM_TREE = 50
CRITERION = 'gini'
MAX_DEPTH = 8
MIN_SAMPLES_SPLIT = 2
MIN_SAMPLES_LEAF = 1
MIN_IMPURITY_DECREASE = 1e-5

if RANDOM_FOREST:
    train_avg.reset(); test_avg.reset()
    for _ in range(NUM_EXPS):
        clf = RandomForestClassifier(n_estimators=NUM_TREE, criterion=CRITERION, max_depth=MAX_DEPTH,
                                     min_samples_split=MIN_SAMPLES_SPLIT, min_samples_leaf=MIN_SAMPLES_LEAF,
                                     min_impurity_decrease=MIN_IMPURITY_DECREASE)
        clf.fit(X_train, y_train)
        train_acc = clf.score(X_train, y_train)
        test_acc = clf.score(X_test, y_test)

        train_avg.update(train_acc)
        test_avg.update(test_acc)

    print(f'Random Forest\t {NUM_EXPS}-Avg train_acc:\t{train_avg.avg:4f}, test_acc:\t{test_avg.avg:4f}')


###########################            GDBT hyper-parameters              ###########################
LOSS = 'deviance'               # (= logistic regression); 'exponential' for AdaBoost
LEARNING_RATE = 0.1
N_ESTIMATORS = 100              # number of boosting stages to perform
SUBSAMPLE = 0.8
CRITERION = 'friedman_mse'      # 'friedman_mse' or 'mse
MIN_SAMPLES_SPLIT = 2
MIN_SAMPLES_LEAF = 1
MIN_IMPURITY_DECREASE = 1e-5
VALIDATION_FRACTION = 0.1       # proportion of training for early stopping

if GBDT:
    train_avg.reset(); test_avg.reset()
    for _ in range(NUM_EXPS):
        clf = GradientBoostingClassifier(loss=LOSS, learning_rate=LEARNING_RATE, n_estimators=N_ESTIMATORS,
                                         subsample=SUBSAMPLE, criterion=CRITERION, min_samples_split=MIN_SAMPLES_SPLIT,
                                         min_samples_leaf=MIN_SAMPLES_LEAF, min_impurity_decrease=MIN_IMPURITY_DECREASE,
                                         validation_fraction=VALIDATION_FRACTION, verbose=True)

        clf.fit(X_train, y_train)
        train_acc = clf.score(X_train, y_train)
        test_acc = clf.score(X_test, y_test)

        train_avg.update(train_acc)
        test_avg.update(test_acc)

    print(f'GBDT\t {NUM_EXPS}-Avg train_acc:\t{train_avg.avg:4f}, test_acc:\t{test_avg.avg:4f}')
