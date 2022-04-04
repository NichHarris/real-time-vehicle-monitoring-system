import pandas
from os import read

columns = [
    'Fuel_Consumption',
    'Engine_Speed',
    'Engine_Coolant_Temperature',
    'Current_Gear',
    'Vehicle_Speed'
]

with open('dataset.csv') as dataset:
    data = pandas.read_csv('dataset.csv', usecols=columns)
    
    for col in columns:
        metric = data[col]
        metric.to_csv(f'{col}.csv', index=0)
