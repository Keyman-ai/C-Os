#pragma once

void  page_alloc_init();
void* alloc_page();
void  free_page(void* ptr);