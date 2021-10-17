#ifndef THREAD_SAFE_LIST_H_
#define THREAD_SAFE_LIST_H_

#include <pthread.h>
#include <iostream>
#include <iomanip> // std::setw

using namespace std;

template <typename T>
class List 
{
    public:
        class Node {
        public:
            T data;
            Node *next;
            pthread_mutex_t m;
            bool is_dummy;

            ~Node(){
                pthread_mutex_destroy(&m);
            }
        };

    private:
        Node* dummy_head;
        Node* head;
        unsigned int size;
        pthread_mutex_t size_m;
        // TODO: Add your own methods and data members

    public:
        /**
         * Constructor
         */
        List() {
            dummy_head = new Node();
            head = nullptr;
            pthread_mutex_init(&dummy_head->m, nullptr);
            dummy_head->is_dummy = true;
            dummy_head->next = nullptr;
            size = 0;
            pthread_mutex_init(&size_m, nullptr);
        }

        /**
         * Destructor
         */
        ~List(){
            pthread_mutex_destroy(&size_m);
            Node *current = head;
            while(current != nullptr){
                Node* to_del = current;
                current = current->next;
                delete to_del;
            }
        }

        /**
         * Insert new node to list while keeping the list ordered in an ascending order
         * If there is already a node has the same data as @param data then return false (without adding it again)
         * @param data the new data to be added to the list
         * @return true if a new node was added and false otherwise
         */
        bool insert(const T& data) {
            Node *new_node = new Node();
			pthread_mutex_init(&new_node->m, nullptr);
			new_node->data = data;
			new_node->is_dummy = false;
			new_node->next = nullptr;

			Node *current = dummy_head;
			Node *prev = nullptr;
			while(current != nullptr){
			    pthread_mutex_lock(&current->m);
			    if(!current->is_dummy && current->data > new_node->data){
                    new_node->next = current;
                    prev->next = new_node;
			        break;
			    }
			    else if(!current->is_dummy && current->data == new_node->data){
			        if(prev!= nullptr){
                        pthread_mutex_unlock(&prev->m);
			        }
                    pthread_mutex_unlock(&current->m);
			        return false;
			    }
			    else{
			        if(prev != nullptr) {
                        pthread_mutex_unlock(&prev->m);
                    }
			        prev = current;
			        current = current->next;
			    }
			}
            if(prev->is_dummy){
                head = new_node;
            }
			if(current== nullptr){
			    // we're here if we didn't inserted the node yet
			    prev->next = new_node;
                __insert_test_hook();
			}
			else{
                __insert_test_hook();
                pthread_mutex_unlock(&current->m);
			}
			pthread_mutex_unlock(&prev->m);
            pthread_mutex_lock(&size_m);
            size++;
            pthread_mutex_unlock(&size_m);
            return true;
        }

        /**
         * Remove the node that its data equals to @param value
         * @param value the data to lookup a node that has the same data to be removed
         * @return true if a matched node was found and removed and false otherwise
         */
        bool remove(const T& value) {
            Node *current = dummy_head;
            Node *prev = nullptr;
            while(current != nullptr){
                pthread_mutex_lock(&current->m);
                if(!current->is_dummy && current->data == value){
                    Node* to_del = current;
                    prev->next = current->next;
                    if(prev->is_dummy){
                        head = prev->next;
                    }
                    __remove_test_hook();
                    pthread_mutex_unlock(&to_del->m);
                    delete to_del;
                    pthread_mutex_unlock(&prev->m);
                    pthread_mutex_lock(&size_m);
                    size--;
                    pthread_mutex_unlock(&size_m);
                    return true;
                }
                else{
                    if(prev != nullptr) {
                        pthread_mutex_unlock(&prev->m);
                    }
                    prev = current;
                    current = current->next;
                }
            }
            pthread_mutex_unlock(&prev->m);
            return false;
        }

        /**
         * Returns the current size of the list
         * @return current size of the list
         */
        unsigned int getSize() {
            return size;
        }

		// Don't remove
        void print() {
          Node* temp = head;
          if (temp == NULL)
          {
            cout << "";
          }
          else if (temp->next == NULL)
          {
            cout << temp->data;
          }
          else
          {
            while (temp != NULL)
            {
              cout << right << setw(3) << temp->data;
              temp = temp->next;
              cout << " ";
            }
          }
          cout << endl;
        }

		// Don't remove
        virtual void __insert_test_hook() {}
		// Don't remove
        virtual void __remove_test_hook() {}
};

#endif //THREAD_SAFE_LIST_H_