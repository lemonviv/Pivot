import numpy as np
import pandas as pd
from sklearn.model_selection import train_test_split

# CreditCard Dataset
def load_CreditCard_dataset(data_path='utils/data/UCI_Credit_Card.csv', test_size=0.20):
    df = pd.read_csv(data_path)
    df = df.rename(columns={'default.payment.next.month': 'def_pay',
                            'PAY_0': 'PAY_1'})
    y = df['def_pay'].copy()
    features = ['LIMIT_BAL', 'SEX', 'EDUCATION', 'MARRIAGE', 'AGE', 'PAY_1', 'PAY_2',
                'PAY_3', 'PAY_4', 'PAY_5', 'PAY_6', 'BILL_AMT1', 'BILL_AMT2',
                'BILL_AMT3', 'BILL_AMT4', 'BILL_AMT5', 'BILL_AMT6', 'PAY_AMT1',
                'PAY_AMT2', 'PAY_AMT3', 'PAY_AMT4', 'PAY_AMT5', 'PAY_AMT6']
    X = df[features].copy()
    X, y = X.to_numpy().astype('float32'), y.to_numpy()
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=test_size)
    return X_train, y_train, X_test, y_test

# Credit Dataset
def load_Credit_dataset(data_path="utils/data/GiveMeSomeCredit/", test_size=0.20):
    def cleaned_dataset(dataset):
        dataset.loc[dataset["age"] <= 18, "age"] = dataset.age.median()

        age_working = dataset.loc[(dataset["age"] >= 18) & (dataset["age"] < 60)]
        age_senior = dataset.loc[(dataset["age"] >= 60)]

        age_working_impute = age_working.MonthlyIncome.mean()
        age_senior_impute = age_senior.MonthlyIncome.mean()

        dataset["MonthlyIncome"] = np.absolute(dataset["MonthlyIncome"])
        dataset["MonthlyIncome"] = dataset["MonthlyIncome"].fillna(99999)
        dataset["MonthlyIncome"] = dataset["MonthlyIncome"].astype('int64')

        dataset.loc[((dataset["age"] >= 18) & (dataset["age"] < 60)) & (dataset["MonthlyIncome"] == 99999), \
                    "MonthlyIncome"] = age_working_impute
        dataset.loc[
            (train_data["age"] >= 60) & (dataset["MonthlyIncome"] == 99999), "MonthlyIncome"] = age_senior_impute
        dataset["NumberOfDependents"] = np.absolute(dataset["NumberOfDependents"])
        dataset["NumberOfDependents"] = dataset["NumberOfDependents"].fillna(0)
        dataset["NumberOfDependents"] = dataset["NumberOfDependents"].astype('int64')

    train_data = pd.read_csv(data_path+'cs-training.csv')
    # test_data = pd.read_csv(data_path'cs-test.csv')
    features = ['RevolvingUtilizationOfUnsecuredLines', 'age', 'NumberOfTime30-59DaysPastDueNotWorse',
                'DebtRatio', 'MonthlyIncome', 'NumberOfOpenCreditLinesAndLoans', 'NumberOfTimes90DaysLate',
                'NumberRealEstateLoansOrLines',
                'NumberOfTime60-89DaysPastDueNotWorse', 'NumberOfDependents']
    cleaned_dataset(train_data)
    # cleaned_dataset(test_data)
    X_train = train_data[features].copy().to_numpy().astype('float32')
    y_train = train_data.SeriousDlqin2yrs.to_numpy().astype('int64')

    X_train, X_test, y_train, y_test = train_test_split(X_train, y_train, test_size=test_size)
    # X_test = test_data[features].copy().to_numpy().astype('float32')
    # y_test = test_data.SeriousDlqin2yrs.to_numpy().astype('int64')
    return X_train, y_train, X_test, y_test
