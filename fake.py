import faker
import random
import csv

fake = faker.Faker()

def generate_fake_data():
    ids = set()
    with open('students.txt', 'w') as f:
        for i in range(10):
            # student id has to be unique
            student_id = fake.random_int(min=100000000, max=999999999)
            while student_id in ids:
                student_id = fake.random_int(min=100000000, max=999999999)
            ids.add(student_id)
            first_name = fake.first_name()
            last_name = fake.last_name()
            course_1 = round(random.uniform(0.0, 4.0), 2)
            course_2 = round(random.uniform(0.0, 4.0), 2)
            course_3 = round(random.uniform(0.0, 4.0), 2)
            course_4 = round(random.uniform(0.0, 4.0), 2)
            course_5 = round(random.uniform(0.0, 4.0), 2)
            course_6 = round(random.uniform(0.0, 4.0), 2)
            course_7 = round(random.uniform(0.0, 4.0), 2)
            course_8 = round(random.uniform(0.0, 4.0), 2)
            gpa = round((course_1 + course_2 + course_3 + course_4 + course_5 + course_6 + course_7 + course_8) / 8, 2)
            f.write(f'{student_id} {first_name} {last_name} {course_1} {course_2} {course_3} {course_4} {course_5} {course_6} {course_7} {course_8} {gpa}\n')

generate_fake_data()