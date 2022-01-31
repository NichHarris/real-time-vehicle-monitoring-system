import pandas
from os import read

columns = [
    'Fuel_consumption',
    'Engine_speed',
    'Engine_coolant_temperature',
    'Current_Gear',
    'Vehicle_speed']


with open('dataset.csv') as dataset:
    data = pandas.read_csv('dataset.csv', usecols=columns)
    
    for col in columns:
        metric = data[col]
        metric.to_csv(f'{col}.csv', index=0)
            
