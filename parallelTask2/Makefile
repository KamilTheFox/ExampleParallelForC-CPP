.PHONY: all clean run_writer run_cancel run_cleanup show

all: background_writer thread_cancel

background_writer:
	gcc -o background_writer background_writer.c -pthread

thread_cancel:
	gcc -o thread_cancel thread_cancel.c -pthread

cleanup_duble:
	gcc -o cleanup_duble cleanup_duble.c -pthread

run_writer: background_writer
	./background_writer

run_cancel: thread_cancel
	./thread_cancel

run_cleanup:
	./cleanup_duble

show:
	cat output.txt

clean:
	rm -f background_writer cleanup_duble thread_cancel output.txt

help:
	@echo "Доступные команды:"
	@echo "  make all         - собрать все программы"
	@echo "  make run_writer  - собрать и запустить фоновую запись"
	@echo "  make run_cancel  - собрать и запустить отмену потока"
	@echo "  make show        - показать содержимое output.txt"
	@echo "  make clean       - очистить все собранные файлы"