/**
 * @file    main.cpp
 * @author  niob
 * @date    Jan 25, 2016
 * @brief   Implements a little test program for the managed heap.
 */

#include <cstddef>
#include <iostream>
#include <utility>

#include "Heap.hpp"
#include "HeapObject.hpp"

// The heap type needs to be specified whenever creating another HeapObject class
using H = ssw::Heap<50 * 1024>;

struct Student;
struct Lecture;

// Managed objects inherit publicly from HeapObject, specifying the heap to allocate from
struct StudentNode : public ssw::HeapObject<StudentNode, H>
{
	// Managed objects must specify a static member variable `type` so inherited operator `new` can be used
	static const ssw::TypeDescriptor &type;
	
	StudentNode *next;
	Student *student;
	
	StudentNode(Student *student, StudentNode *next = nullptr)
			: next(next), student(student) {
	}
};

// Use TypeDescriptor::make for creating the type descriptor object, which needs to be allocated dynamically
const ssw::TypeDescriptor &StudentNode::type = *ssw::TypeDescriptor::make<StudentNode>({
	// This only works with standard-layout types unfortunately
	offsetof(StudentNode, next),
	offsetof(StudentNode, student)
});

struct LectureNode : public ssw::HeapObject<LectureNode, H>
{
	static const ssw::TypeDescriptor &type;
	
	LectureNode *next;
	Lecture *lecture;
	
	LectureNode(Lecture *lecture, LectureNode *next = nullptr)
			: next(next), lecture(lecture) {
	}
};

const ssw::TypeDescriptor &LectureNode::type = *ssw::TypeDescriptor::make<LectureNode>({
	offsetof(LectureNode, next),
	offsetof(LectureNode, lecture)
});

struct StudentList : public ssw::HeapObject<StudentList, H>
{
	static const ssw::TypeDescriptor &type;
	
	StudentNode *first = nullptr;
	
	void add(Student *student) {
		first = new StudentNode(student, first);
	}
	
	void remove(Student *student) {
		StudentNode *prev = nullptr;
		for(StudentNode *it = first; it; prev = std::exchange(it, it->next)) {
			if(it->student == student) {
				if(prev) {
					prev->next = it->next;
				} else {
					first = it->next;
				}
			}
		}
	}
};

const ssw::TypeDescriptor &StudentList::type = *ssw::TypeDescriptor::make<StudentList>({
	offsetof(StudentList, first)
});

struct Student : public ssw::HeapObject<Student, H>
{
	static const ssw::TypeDescriptor &type;
	
	int id;
	const char *name;
	LectureNode *lectures;
	
	Student(int id, const char *name)
			: id(id), name(name), lectures(nullptr) {
	}
	
	void add(Lecture *lecture) {
		lectures = new LectureNode(lecture, lectures);
	}
	
	void remove(Lecture *lecture) {
		LectureNode *prev;
		for(LectureNode *it = lectures; it; prev = std::exchange(it, it->next)) {
			if(it->lecture == lecture) {
				if(prev) {
					prev->next = it->next;
				} else {
					lectures = it->next;
				}
			}
		}
	}
};

const ssw::TypeDescriptor &Student::type = *ssw::TypeDescriptor::make<Student>({
	offsetof(Student, lectures)
});

struct Lecture : public ssw::HeapObject<Lecture, H>
{
	static const ssw::TypeDescriptor &type;
	
	int id;
	const char *name;
	int semester;
	
	Lecture(int id, const char *name, int sem)
			: id(id), name(name), semester(sem) {
	}
};

const ssw::TypeDescriptor &Lecture::type = *ssw::TypeDescriptor::make<Lecture>();

int main(int, char**) {
	std::cout << "Heap after creation without anything allocated yet:\n";
	// The heap is implemented as a singleton, Heap::instance gets the instance
	H::instance().dump(std::cout);
	
	StudentList *list = new(true) StudentList();
	
	Lecture *ssw = new Lecture{1, "System Software", 7};
	Lecture *popl = new Lecture{2, "Principles of Programming Languages", 7};
	Lecture *re = new Lecture{3, "Requirements Engineering", 7};
	
	Student *peter = new Student{1, "Peter Feichtinger"};
	list->add(peter);
	Student *latifi = new Student{2, "Florian Latifi"};
	list->add(latifi);
	Student *daniel = new Student{3, "Daniel Hinterreiter"};
	
	peter->add(ssw);
	peter->add(popl);
	peter->add(re);
	latifi->add(popl);
	latifi->add(re);
	daniel->add(ssw);
	daniel->add(re);
	
	list->add(daniel);
	
	std::cout << "Heap after allocating some objects, all still alive:\n";
	H::instance().dump(std::cout);
	list->remove(daniel);
	peter->remove(ssw);
	std::cout << "Heap after some objects died, but before garbage collection:\n";
	H::instance().dump(std::cout);
	H::instance().gc();
	std::cout << "Heap after garbage collection:\n";
	H::instance().dump(std::cout);
	H::instance().removeRoot(list);
	H::instance().gc();
	std::cout << "Heap after removing the single root pointer and performing GC:\n";
	H::instance().dump(std::cout);
}
