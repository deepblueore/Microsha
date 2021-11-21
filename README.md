# Microsha
Компиляция:  
g++ microsha2.cpp -o microsha_valgrind (./microsha_valgrind для запуска с valgrind)  
g++ -fsanitize=address microsha2.cpp -o microsha_fsanitize (./microsha_fsanitize для запуска с -fsanitize=address)  

Не работает ls -l /usr/include/* .h  
Не реализованы echo и set
