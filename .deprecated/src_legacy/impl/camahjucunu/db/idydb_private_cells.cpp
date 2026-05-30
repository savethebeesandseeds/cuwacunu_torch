static unsigned char idydb_insert_value_int(idydb **handler, int set_value)
{
	if (!handler || !*handler) return IDYDB_ERROR;
	if (!(*handler)->configured) { idydb_error_state(handler, 8); return IDYDB_ERROR; }
	if ((*handler)->read_only != IDYDB_READ_AND_WRITE) { idydb_error_state(handler, 9); return IDYDB_READONLY; }
	if ((*handler)->value_type != IDYDB_NULL && !(*handler)->value_retrieved) { idydb_error_state(handler, 10); return IDYDB_ERROR; }
	idydb_clear_values(handler);
	(*handler)->value_type = IDYDB_INTEGER;
	(*handler)->value.int_value = set_value;
	return IDYDB_DONE;
}

static unsigned char idydb_insert_value_float(idydb **handler, float set_value)
{
	if (!handler || !*handler) return IDYDB_ERROR;
	if (!(*handler)->configured) { idydb_error_state(handler, 8); return IDYDB_ERROR; }
	if ((*handler)->read_only != IDYDB_READ_AND_WRITE) { idydb_error_state(handler, 9); return IDYDB_READONLY; }
	if ((*handler)->value_type != IDYDB_NULL && !(*handler)->value_retrieved) { idydb_error_state(handler, 10); return IDYDB_ERROR; }
	idydb_clear_values(handler);
	(*handler)->value_type = IDYDB_FLOAT;
	(*handler)->value.float_value = set_value;
	return IDYDB_DONE;
}

static unsigned char idydb_insert_value_char(idydb **handler, char *set_value)
{
	if (!handler || !*handler) return IDYDB_ERROR;
	if (!set_value) { idydb_error_state(handler, 8); return IDYDB_ERROR; }
	if (!(*handler)->configured) { idydb_error_state(handler, 8); return IDYDB_ERROR; }
	if ((*handler)->read_only != IDYDB_READ_AND_WRITE) { idydb_error_state(handler, 9); return IDYDB_READONLY; }
	if ((*handler)->value_type != IDYDB_NULL && !(*handler)->value_retrieved) { idydb_error_state(handler, 10); return IDYDB_ERROR; }
	idydb_clear_values(handler);
	(*handler)->value_type = IDYDB_CHAR;
	strncpy((*handler)->value.char_value, set_value, sizeof((*handler)->value.char_value));
	return IDYDB_DONE;
}

static unsigned char idydb_insert_value_bool(idydb **handler, bool set_value)
{
	if (!handler || !*handler) return IDYDB_ERROR;
	if (!(*handler)->configured) { idydb_error_state(handler, 8); return IDYDB_ERROR; }
	if ((*handler)->read_only != IDYDB_READ_AND_WRITE) { idydb_error_state(handler, 9); return IDYDB_READONLY; }
	if ((*handler)->value_type != IDYDB_NULL && !(*handler)->value_retrieved) { idydb_error_state(handler, 10); return IDYDB_ERROR; }
	idydb_clear_values(handler);
	(*handler)->value_type = IDYDB_BOOL;
	(*handler)->value.bool_value = set_value;
	return IDYDB_DONE;
}

static unsigned char idydb_insert_value_vector(idydb **handler, const float* data, unsigned short dims)
{
	if (!handler || !*handler) return IDYDB_ERROR;
	if (!data) { idydb_error_state(handler, 8); return IDYDB_ERROR; }
	if (!(*handler)->configured) { idydb_error_state(handler, 8); return IDYDB_ERROR; }
	if ((*handler)->read_only != IDYDB_READ_AND_WRITE) { idydb_error_state(handler, 9); return IDYDB_READONLY; }
	if ((*handler)->value_type != IDYDB_NULL && !(*handler)->value_retrieved) { idydb_error_state(handler, 10); return IDYDB_ERROR; }
	if (dims == 0 || dims > IDYDB_MAX_VECTOR_DIM) { idydb_error_state(handler, 11); return IDYDB_ERROR; }
	idydb_clear_values(handler);
	(*handler)->value_type  = IDYDB_VECTOR;
	(*handler)->vector_dims = dims;
	(*handler)->vector_value = (float*)malloc(sizeof(float) * (size_t)dims);
	if (!(*handler)->vector_value) { idydb_error_state(handler, 24); return IDYDB_ERROR; }
	memcpy((*handler)->vector_value, data, sizeof(float) * (size_t)dims);
	return IDYDB_DONE;
}

static void idydb_insert_reset(idydb **handler)
{
	if (!handler || !*handler) return;
	idydb_clear_values(handler);
}

/* ---------------- Public inserts ---------------- */

int idydb_insert_int(idydb **handler, idydb_column_row_sizing c, idydb_column_row_sizing r, int value)
{
	unsigned char s = idydb_insert_value_int(handler, value);
	if (s != IDYDB_DONE) return s;
	return idydb_insert_at(handler, c, r);
}

int idydb_insert_float(idydb **handler, idydb_column_row_sizing c, idydb_column_row_sizing r, float value)
{
	unsigned char s = idydb_insert_value_float(handler, value);
	if (s != IDYDB_DONE) return s;
	return idydb_insert_at(handler, c, r);
}

int idydb_insert_char(idydb **handler, idydb_column_row_sizing c, idydb_column_row_sizing r, char *value)
{
	if (!handler || !*handler || !value) { idydb_error_state_if_available(handler, 8); return IDYDB_ERROR; }
	unsigned int value_length = (unsigned int)strlen(value);
	/* reader requires stored_len+1 <= IDYDB_MAX_CHAR_LENGTH => stored_len <= IDYDB_MAX_CHAR_LENGTH-1 */
	if (value_length >= IDYDB_MAX_CHAR_LENGTH)
	{
		idydb_error_state(handler, 11);
		return IDYDB_ERROR;
	}
	unsigned char s = idydb_insert_value_char(handler, value);
	if (s != IDYDB_DONE) return s;
	return idydb_insert_at(handler, c, r);
}

int idydb_insert_const_char(idydb **handler, idydb_column_row_sizing c, idydb_column_row_sizing r, const char *value)
{
	if (!handler || !*handler || !value) { idydb_error_state_if_available(handler, 8); return IDYDB_ERROR; }
	unsigned int value_length = (unsigned int)strlen(value);
	if (value_length >= IDYDB_MAX_CHAR_LENGTH)
	{
		idydb_error_state(handler, 11);
		return IDYDB_ERROR;
	}

	char* tmp = (char*)malloc((size_t)value_length + 1);
	if (!tmp) { idydb_error_state(handler, 24); return IDYDB_ERROR; }
	memcpy(tmp, value, (size_t)value_length + 1);
	int rc = idydb_insert_char(handler, c, r, tmp);
	free(tmp);
	return rc;
}

int idydb_insert_bool(idydb **handler, idydb_column_row_sizing c, idydb_column_row_sizing r, bool value)
{
	unsigned char s = idydb_insert_value_bool(handler, (value == true));
	if (s != IDYDB_DONE) return s;
	return idydb_insert_at(handler, c, r);
}

int idydb_insert_vector(idydb **handler, idydb_column_row_sizing c, idydb_column_row_sizing r, const float* data, unsigned short dims)
{
	unsigned char s = idydb_insert_value_vector(handler, data, dims);
	if (s != IDYDB_DONE) return s;
	return idydb_insert_at(handler, c, r);
}

static int idydb_bulk_cell_compare(const void* lhs, const void* rhs)
{
	const idydb_bulk_cell* a = (const idydb_bulk_cell*)lhs;
	const idydb_bulk_cell* b = (const idydb_bulk_cell*)rhs;
	if (a->column < b->column) return -1;
	if (a->column > b->column) return 1;
	if (a->row < b->row) return -1;
	if (a->row > b->row) return 1;
	return 0;
}

static int idydb_bulk_cell_validate(const idydb_bulk_cell* cell)
{
	if (!cell) return IDYDB_ERROR;
	if (cell->column == 0 || cell->row == 0) return IDYDB_RANGE;
	switch (cell->type)
	{
	case IDYDB_INTEGER:
	case IDYDB_FLOAT:
	case IDYDB_BOOL:
		return IDYDB_SUCCESS;
	case IDYDB_CHAR:
		if (!cell->value.s) return IDYDB_ERROR;
		if (strlen(cell->value.s) >= IDYDB_MAX_CHAR_LENGTH) return IDYDB_RANGE;
		return IDYDB_SUCCESS;
	case IDYDB_VECTOR:
		if (!cell->value.vec || cell->vector_dims == 0 || cell->vector_dims > IDYDB_MAX_VECTOR_DIM) {
			return IDYDB_ERROR;
		}
		return IDYDB_SUCCESS;
	default:
		return IDYDB_ERROR;
	}
}

int idydb_replace_with_cells(idydb **handler,
                             const idydb_bulk_cell* cells,
                             size_t cell_count)
{
	if (!handler || !*handler) return IDYDB_ERROR;
	if (!(*handler)->configured) { idydb_error_state(handler, 8); return IDYDB_ERROR; }
	if ((*handler)->read_only != IDYDB_READ_AND_WRITE) {
		idydb_error_state(handler, 9);
		idydb_clear_values(handler);
		return IDYDB_READONLY;
	}
	if (cell_count != 0 && !cells) {
		idydb_error_state(handler, 8);
		return IDYDB_ERROR;
	}

	idydb_bulk_cell* sorted = NULL;
	if (cell_count != 0) {
		sorted = (idydb_bulk_cell*)malloc(cell_count * sizeof(idydb_bulk_cell));
		if (!sorted) {
			idydb_error_state(handler, 24);
			return IDYDB_ERROR;
		}
		memcpy(sorted, cells, cell_count * sizeof(idydb_bulk_cell));
		qsort(sorted, cell_count, sizeof(idydb_bulk_cell), idydb_bulk_cell_compare);
	}

	FILE* file = (*handler)->file_descriptor;
	if (!file) {
		if (sorted) free(sorted);
		idydb_error_state(handler, 5);
		return IDYDB_ERROR;
	}

	const int fd = fileno(file);
	if (fd < 0 || ftruncate(fd, 0) != 0 || fseek(file, 0L, SEEK_SET) != 0) {
		if (sorted) free(sorted);
		idydb_error_state(handler, 15);
		return IDYDB_ERROR;
	}

	idydb_column_row_sizing previous_column = 0;
	for (size_t index = 0; index < cell_count;) {
		const idydb_bulk_cell* head = &sorted[index];
		const int validation_rc = idydb_bulk_cell_validate(head);
		if (validation_rc != IDYDB_SUCCESS) {
			if (sorted) free(sorted);
			idydb_error_state(handler, validation_rc == IDYDB_RANGE ? 22 : 20);
			return validation_rc == IDYDB_RANGE ? IDYDB_RANGE : IDYDB_ERROR;
		}
		if (head->column <= previous_column) {
			if (sorted) free(sorted);
			idydb_error_state(handler, 22);
			return IDYDB_RANGE;
		}

		size_t end = index;
		while (end < cell_count && sorted[end].column == head->column) {
			const int row_validation_rc = idydb_bulk_cell_validate(&sorted[end]);
			if (row_validation_rc != IDYDB_SUCCESS) {
				if (sorted) free(sorted);
				idydb_error_state(handler, row_validation_rc == IDYDB_RANGE ? 22 : 20);
				return row_validation_rc == IDYDB_RANGE ? IDYDB_RANGE : IDYDB_ERROR;
			}
			if (end > index && sorted[end - 1].row >= sorted[end].row) {
				if (sorted) free(sorted);
				idydb_error_state(handler, 22);
				return IDYDB_RANGE;
			}
			++end;
		}

		const idydb_column_row_sizing skip_amount_raw =
			head->column - previous_column - 1;
		const idydb_column_row_sizing row_count_raw =
			(idydb_column_row_sizing)(end - index - 1);
		if (skip_amount_raw > 0xFFFFu || row_count_raw > 0xFFFFu) {
			if (sorted) free(sorted);
			idydb_error_state(handler, 22);
			return IDYDB_RANGE;
		}

		const unsigned short skip_amount = (unsigned short)skip_amount_raw;
		const unsigned short row_count = (unsigned short)row_count_raw;
		if (fwrite(&skip_amount, sizeof(short), 1, file) != 1 ||
		    fwrite(&row_count, sizeof(short), 1, file) != 1) {
			if (sorted) free(sorted);
			idydb_error_state(handler, 15);
			return IDYDB_ERROR;
		}

		for (size_t i = index; i < end; ++i) {
			const idydb_bulk_cell* cell = &sorted[i];
			const idydb_column_row_sizing row_index_raw = cell->row - 1;
			if (row_index_raw > 0xFFFFu) {
				if (sorted) free(sorted);
				idydb_error_state(handler, 22);
				return IDYDB_RANGE;
			}
			const unsigned short row_index = (unsigned short)row_index_raw;
			unsigned char tag = 0;
			switch (cell->type) {
			case IDYDB_INTEGER: tag = IDYDB_READ_INT; break;
			case IDYDB_FLOAT: tag = IDYDB_READ_FLOAT; break;
			case IDYDB_CHAR: tag = IDYDB_READ_CHAR; break;
			case IDYDB_BOOL: tag = cell->value.b ? IDYDB_READ_BOOL_TRUE : IDYDB_READ_BOOL_FALSE; break;
			case IDYDB_VECTOR: tag = IDYDB_READ_VECTOR; break;
			default:
				if (sorted) free(sorted);
				idydb_error_state(handler, 20);
				return IDYDB_ERROR;
			}

			if (fwrite(&row_index, sizeof(short), 1, file) != 1 ||
			    fwrite(&tag, 1, 1, file) != 1) {
				if (sorted) free(sorted);
				idydb_error_state(handler, 15);
				return IDYDB_ERROR;
			}

			switch (cell->type) {
			case IDYDB_INTEGER:
				if (fwrite(&cell->value.i, sizeof(int), 1, file) != 1) {
					if (sorted) free(sorted);
					idydb_error_state(handler, 15);
					return IDYDB_ERROR;
				}
				break;
			case IDYDB_FLOAT:
				if (fwrite(&cell->value.f, sizeof(float), 1, file) != 1) {
					if (sorted) free(sorted);
					idydb_error_state(handler, 15);
					return IDYDB_ERROR;
				}
				break;
			case IDYDB_CHAR:
			{
				const unsigned short len = (unsigned short)strlen(cell->value.s);
				if (fwrite(&len, sizeof(short), 1, file) != 1 ||
				    fwrite(cell->value.s, 1, (size_t)len + 1u, file) != (size_t)len + 1u) {
					if (sorted) free(sorted);
					idydb_error_state(handler, 15);
					return IDYDB_ERROR;
				}
				break;
			}
			case IDYDB_BOOL:
				break;
			case IDYDB_VECTOR:
				if (fwrite(&cell->vector_dims, sizeof(short), 1, file) != 1 ||
				    fwrite(cell->value.vec, sizeof(float), cell->vector_dims, file) != cell->vector_dims) {
					if (sorted) free(sorted);
					idydb_error_state(handler, 15);
					return IDYDB_ERROR;
				}
				break;
			}
		}

		previous_column = head->column;
		index = end;
	}

	if (fflush(file) != 0 || fseek(file, 0L, SEEK_END) != 0) {
		if (sorted) free(sorted);
		idydb_error_state(handler, 15);
		return IDYDB_ERROR;
	}
	const long final_size = ftell(file);
	if (final_size < 0 || fseek(file, 0L, SEEK_SET) != 0) {
		if (sorted) free(sorted);
		idydb_error_state(handler, 15);
		return IDYDB_ERROR;
	}

	(*handler)->size = (idydb_sizing_max)final_size;
	(*handler)->dirty = true;
	idydb_clear_values(handler);
	if (sorted) free(sorted);
	return IDYDB_SUCCESS;
}

int idydb_delete(idydb **handler, idydb_column_row_sizing c, idydb_column_row_sizing r)
{
	idydb_insert_reset(handler);
	return idydb_insert_at(handler, c, r);
}

/* ---------------- retrieve staged value ---------------- */

static int idydb_retrieve_value_int(idydb **handler)
{
	if (!handler || !*handler) return 0;
	return ((*handler)->value_type == IDYDB_INTEGER) ? (*handler)->value.int_value : 0;
}
static float idydb_retrieve_value_float(idydb **handler)
{
	if (!handler || !*handler) return 0.0f;
	return ((*handler)->value_type == IDYDB_FLOAT) ? (*handler)->value.float_value : 0.0f;
}
static char *idydb_retrieve_value_char(idydb **handler)
{
	if (!handler || !*handler) return NULL;
	return ((*handler)->value_type == IDYDB_CHAR) ? (*handler)->value.char_value : NULL;
}
static bool idydb_retrieve_value_bool(idydb **handler)
{
	if (!handler || !*handler) return false;
	return ((*handler)->value_type == IDYDB_BOOL) ? (*handler)->value.bool_value : false;
}

static const float* idydb_retrieve_value_vector(idydb **handler, unsigned short* out_dims)
{
	if (!handler || !*handler) {
		if (out_dims) *out_dims = 0;
		return NULL;
	}
	if ((*handler)->value_type == IDYDB_VECTOR) {
		if (out_dims) *out_dims = (*handler)->vector_dims;
		return (*handler)->vector_value;
	}
	if (out_dims) *out_dims = 0;
	return NULL;
}

static unsigned char idydb_retrieve_value_type(idydb **handler)
{
	if (!handler || !*handler) return IDYDB_NULL;
	return (*handler)->value_type;
}

int idydb_retrieve_int(idydb **handler) { return idydb_retrieve_value_int(handler); }
float idydb_retrieve_float(idydb **handler) { return idydb_retrieve_value_float(handler); }
char *idydb_retrieve_char(idydb **handler) { return idydb_retrieve_value_char(handler); }
bool idydb_retrieve_bool(idydb **handler) { return idydb_retrieve_value_bool(handler); }
const float* idydb_retrieve_vector(idydb **handler, unsigned short* out_dims) { return idydb_retrieve_value_vector(handler, out_dims); }

/* ---------------- read value at (column,row) ---------------- */
/* This function is your original logic (unchanged). */

static unsigned char idydb_read_at(idydb **handler, idydb_column_row_sizing column_position, idydb_column_row_sizing row_position)
{
	if (!handler || !*handler) return IDYDB_ERROR;
	if (!(*handler)->configured)
	{
		idydb_error_state(handler, 8);
		return IDYDB_ERROR;
	}
	idydb_clear_values(handler);
#ifdef IDYDB_ALLOW_UNSAFE
	if (!(*handler)->unsafe)
	{
#endif
		if (column_position == 0 || (column_position - 1) > IDYDB_COLUMN_POSITION_MAX || row_position == 0 || (row_position - 1) > IDYDB_ROW_POSITION_MAX)
		{
			idydb_error_state(handler, 12);
			idydb_clear_values(handler);
			return IDYDB_RANGE;
		}
#ifdef IDYDB_ALLOW_UNSAFE
	}
	else
#endif
		if (column_position == 0 || row_position == 0)
	{
		idydb_error_state(handler, 21);
		idydb_clear_values(handler);
		return IDYDB_RANGE;
	}
	else if ((row_position - 1) > IDYDB_ROW_POSITION_MAX)
	{
		idydb_error_state(handler, 12);
		idydb_clear_values(handler);
		return IDYDB_RANGE;
	}
	row_position -= 1;
	bool store_response = false;
	idydb_sizing_max offset = 0;
	idydb_size_selection_type skip_offset = 0;
	unsigned char read_length = IDYDB_PARTITION_AND_SEGMENT;
	unsigned short row_count = 0;
	for (;;)
	{
		if ((offset + read_length) > (*handler)->size)
		{
			if ((offset + read_length) > ((*handler)->size + read_length))
			{
				idydb_error_state(handler, 13);
				return IDYDB_CORRUPT;
			}
			break;
		}
#ifdef IDYDB_MMAP_OK
		if ((*handler)->read_only != IDYDB_READONLY_MMAPPED)
		{
#endif
			fseek((*handler)->file_descriptor, offset, SEEK_SET);
			offset += read_length;
#ifdef IDYDB_MMAP_OK
		}
#endif
		if (read_length == IDYDB_PARTITION_AND_SEGMENT)
		{
			unsigned short skip_amount;
#ifdef IDYDB_MMAP_OK
			if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
				skip_amount = (unsigned short)idydb_read_mmap(offset, sizeof(short), (*handler)->buffer).integer;
			else
#endif
				if (fread(&skip_amount, 1, sizeof(short), (*handler)->file_descriptor) != sizeof(short))
			{
				idydb_error_state(handler, 14);
				return IDYDB_ERROR;
			}
			skip_offset += skip_amount;
			if (skip_offset > IDYDB_COLUMN_POSITION_MAX)
			{
#ifdef IDYDB_ALLOW_UNSAFE
				if (!(*handler)->unsafe)
				{
#endif
					idydb_error_state(handler, 22);
					return IDYDB_RANGE;
#ifdef IDYDB_ALLOW_UNSAFE
				}
#endif
			}
			skip_offset += 1;
			if (skip_offset > column_position)
				return IDYDB_NULL;
#ifdef IDYDB_MMAP_OK
			if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
				row_count = (unsigned short)idydb_read_mmap((offset + sizeof(short)), sizeof(short), (*handler)->buffer).integer;
			else
#endif
				if (fread(&row_count, 1, sizeof(short), (*handler)->file_descriptor) != sizeof(short))
			{
				idydb_error_state(handler, 14);
				return IDYDB_ERROR;
			}
#if IDYDB_SIZING_MODE != IDYDB_SIZING_MODE_BIG
			if (row_count > IDYDB_ROW_POSITION_MAX)
			{
				idydb_error_state(handler, 22);
				return IDYDB_RANGE;
			}
#endif
			row_count += 1;
		}
		unsigned char set_read_length = IDYDB_PARTITION_AND_SEGMENT;
		if (skip_offset == column_position)
		{
			unsigned short position;
#ifdef IDYDB_MMAP_OK
			if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
				position = (unsigned short)idydb_read_mmap((offset + ((sizeof(short) * 2) * (read_length == IDYDB_PARTITION_AND_SEGMENT))), sizeof(short), (*handler)->buffer).integer;
			else
#endif
				if (fread(&position, 1, sizeof(short), (*handler)->file_descriptor) != sizeof(short))
			{
				idydb_error_state(handler, 14);
				return IDYDB_ERROR;
			}
			if (position == row_position)
			{
				store_response = true;
				row_count = 0;
			}
#if IDYDB_SIZING_MODE != IDYDB_SIZING_MODE_BIG
			else if (position > IDYDB_ROW_POSITION_MAX)
			{
				idydb_error_state(handler, 22);
				return IDYDB_RANGE;
			}
#endif
		}
		else
#ifdef IDYDB_MMAP_OK
			if ((*handler)->read_only != IDYDB_READONLY_MMAPPED)
#endif
			fseek((*handler)->file_descriptor, sizeof(short), SEEK_CUR);
		if (row_count > 1)
		{
			row_count -= 1;
			set_read_length = IDYDB_SEGMENT_SIZE;
		}
		unsigned short response_length;
		unsigned char data_type;
#ifdef IDYDB_MMAP_OK
		idydb_sizing_max offset_mmap_standard_diff = offset + (sizeof(short) * 3);
		if (read_length == IDYDB_SEGMENT_SIZE)
			offset_mmap_standard_diff = (offset + sizeof(short));
		if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
		{
			if (read_length == IDYDB_PARTITION_AND_SEGMENT)
				data_type = (unsigned short)idydb_read_mmap((offset + (sizeof(short) * 3)), 1, (*handler)->buffer).integer;
			else
				data_type = (unsigned short)idydb_read_mmap((offset + sizeof(short)), 1, (*handler)->buffer).integer;
			offset_mmap_standard_diff += 1;
		}
		else
#endif
			if (fread(&data_type, 1, 1, (*handler)->file_descriptor) != 1)
		{
			idydb_error_state(handler, 14);
			return IDYDB_ERROR;
		}
		(*handler)->value_retrieved = store_response;
		switch (data_type)
		{
		case IDYDB_READ_INT:
			if (store_response)
			{
				(*handler)->value_type = IDYDB_INTEGER;
#ifdef IDYDB_MMAP_OK
				if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
					(*handler)->value.int_value = idydb_read_mmap(offset_mmap_standard_diff, sizeof(int), (*handler)->buffer).integer;
				else
#endif
					if (fread(&(*handler)->value.int_value, 1, sizeof(int), (*handler)->file_descriptor) != sizeof(int))
				{
					idydb_error_state(handler, 18);
					return IDYDB_ERROR;
				}
				return IDYDB_DONE;
			}
			data_type = IDYDB_INTEGER;
			response_length = sizeof(int);
			break;
		case IDYDB_READ_FLOAT:
			if (store_response)
			{
				(*handler)->value_type = IDYDB_FLOAT;
#ifdef IDYDB_MMAP_OK
				if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
					(*handler)->value.float_value = idydb_read_mmap(offset_mmap_standard_diff, sizeof(float), (*handler)->buffer).floating_point;
				else
#endif
					if (fread(&(*handler)->value.float_value, 1, sizeof(float), (*handler)->file_descriptor) != sizeof(float))
				{
					idydb_error_state(handler, 18);
					return IDYDB_ERROR;
				}
				return IDYDB_DONE;
			}
			data_type = IDYDB_FLOAT;
			response_length = sizeof(float);
			break;
		case IDYDB_READ_CHAR:
			data_type = IDYDB_CHAR;
#ifdef IDYDB_MMAP_OK
			if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
				response_length = (unsigned short)idydb_read_mmap(offset_mmap_standard_diff, sizeof(short), (*handler)->buffer).integer;
			else
#endif
				if (fread(&response_length, 1, sizeof(short), (*handler)->file_descriptor) != sizeof(short))
			{
				idydb_error_state(handler, 18);
				return IDYDB_ERROR;
			}
			response_length += 1;
			if (response_length > IDYDB_MAX_CHAR_LENGTH)
			{
				idydb_error_state(handler, 19);
				return IDYDB_ERROR;
			}
			if (store_response)
			{
				(*handler)->value_type = data_type;
				memset((*handler)->value.char_value, 0, sizeof((*handler)->value.char_value));
#ifdef IDYDB_MMAP_OK
				if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
				{
					idydb_sizing_max offset_diff = (offset_mmap_standard_diff + sizeof(short));
					for (idydb_sizing_max i = 0; i < response_length; i++)
						(*handler)->value.char_value[i] = ((char *)(*handler)->buffer)[(i + offset_diff)];
				}
				else
#endif
				{
					if (fread((*handler)->value.char_value, sizeof(char), response_length, (*handler)->file_descriptor) != response_length)
					{
						idydb_error_state(handler, 18);
						return IDYDB_ERROR;
					}
				}
				return IDYDB_DONE;
			}
			response_length += sizeof(short);
			break;
		case IDYDB_READ_BOOL_TRUE:
		case IDYDB_READ_BOOL_FALSE:
			if (store_response)
			{
				(*handler)->value_type = IDYDB_BOOL;
				(*handler)->value.bool_value = (data_type == IDYDB_READ_BOOL_TRUE);
				return IDYDB_DONE;
			}
			data_type = IDYDB_BOOL;
			response_length = 0;
			break;
		case IDYDB_READ_VECTOR:
		{
			data_type = IDYDB_VECTOR;
			unsigned short dims = 0;
#ifdef IDYDB_MMAP_OK
			if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
				dims = (unsigned short)idydb_read_mmap(offset_mmap_standard_diff, sizeof(short), (*handler)->buffer).integer;
			else
#endif
				if (fread(&dims, 1, sizeof(short), (*handler)->file_descriptor) != sizeof(short))
			{
				idydb_error_state(handler, 18);
				return IDYDB_ERROR;
			}
			if (dims == 0 || dims > IDYDB_MAX_VECTOR_DIM)
			{
				idydb_error_state(handler, 19);
				return IDYDB_ERROR;
			}
			unsigned short bytes = (unsigned short)(dims * sizeof(float));
			if (store_response)
			{
				(*handler)->value_type  = data_type;
				(*handler)->vector_dims = dims;
				(*handler)->vector_value = (float*)malloc(sizeof(float) * (size_t)dims);
				if (!(*handler)->vector_value)
				{
					idydb_error_state(handler, 24);
					return IDYDB_ERROR;
				}
#ifdef IDYDB_MMAP_OK
				if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
				{
					idydb_sizing_max base = (offset_mmap_standard_diff + sizeof(short));
					for (unsigned short i = 0; i < dims; ++i)
					{
						union idydb_read_mmap_response f =
							idydb_read_mmap((unsigned int)(base + (i * sizeof(float))), (unsigned char)sizeof(float), (*handler)->buffer);
						(*handler)->vector_value[i] = f.floating_point;
					}
				}
				else
#endif
				{
					if (fread((*handler)->vector_value, sizeof(float), dims, (*handler)->file_descriptor) != dims)
					{
						idydb_error_state(handler, 18);
						return IDYDB_ERROR;
					}
				}
				return IDYDB_DONE;
			}
			response_length = bytes + sizeof(short);
			break;
		}
		default:
			(*handler)->value_retrieved = false;
			idydb_error_state(handler, 20);
			return IDYDB_CORRUPT;
		}
#ifdef IDYDB_MMAP_OK
		if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
			offset += read_length;
#endif
		read_length = set_read_length;
		offset += response_length;
	}
	return IDYDB_NULL;
}

/* ---------------- insert value at (column,row) ---------------- */
/* This is your original insert logic, with:
 *  - CHAR payload sizing fixed (includes '\0')
 *  - length field written correctly
 *  - dirty flag set on success
 */

static unsigned char idydb_insert_at(idydb **handler, idydb_column_row_sizing column_position, idydb_column_row_sizing row_position)
{
	if (!handler || !*handler) return IDYDB_ERROR;
	if (!(*handler)->configured)
	{
		idydb_error_state(handler, 8);
		return IDYDB_ERROR;
	}
	if ((*handler)->read_only != IDYDB_READ_AND_WRITE)
	{
		idydb_error_state(handler, 9);
		idydb_clear_values(handler);
		return IDYDB_READONLY;
	}
#ifdef IDYDB_ALLOW_UNSAFE
	if (!(*handler)->unsafe)
	{
#endif
		if (column_position == 0 || (column_position - 1) > IDYDB_COLUMN_POSITION_MAX || row_position == 0 || (row_position - 1) > IDYDB_ROW_POSITION_MAX)
		{
			idydb_error_state(handler, 12);
			idydb_clear_values(handler);
			return IDYDB_RANGE;
		}
#ifdef IDYDB_ALLOW_UNSAFE
	}
	else
#endif
		if (column_position == 0 || row_position == 0)
	{
		idydb_error_state(handler, 21);
		idydb_clear_values(handler);
		return IDYDB_RANGE;
	}
	else if ((row_position - 1) > IDYDB_ROW_POSITION_MAX)
	{
		idydb_error_state(handler, 12);
		idydb_clear_values(handler);
		return IDYDB_RANGE;
	}

#if CUWACUNU_CAMAHJUCUNU_DB_VERBOSE_DEBUG
	/* Keep original coords for debug */
	idydb_column_row_sizing dbg_col = column_position;
	idydb_column_row_sizing dbg_row = row_position;

	/* Capture before/after in a way that doesn't break the staged write. */
	idydb_sizing_max dbg_size_before = (*handler)->size;

	char dbg_before[256]; dbg_before[0] = '\0';
	char dbg_after[256];  dbg_after[0]  = '\0';
	bool dbg_have = true;

	/* Preserve staged payload so we can peek old cell value safely. */
	unsigned char dbg_stage_type = (*handler)->value_type;

	int   dbg_i = 0;
	float dbg_f = 0.0f;
	bool  dbg_b = false;

	char* dbg_sdup = NULL;

	float* dbg_vec_ptr = NULL;
	unsigned short dbg_vec_dims = 0;

	if (dbg_stage_type == IDYDB_INTEGER) dbg_i = (*handler)->value.int_value;
	else if (dbg_stage_type == IDYDB_FLOAT) dbg_f = (*handler)->value.float_value;
	else if (dbg_stage_type == IDYDB_BOOL) dbg_b = (*handler)->value.bool_value;
	else if (dbg_stage_type == IDYDB_CHAR)
	{
		size_t n = strlen((*handler)->value.char_value);
		dbg_sdup = (char*)malloc(n + 1);
		if (dbg_sdup) memcpy(dbg_sdup, (*handler)->value.char_value, n + 1);
		else dbg_have = false; /* can't safely peek without losing staged string */
	}
	else if (dbg_stage_type == IDYDB_VECTOR)
	{
		dbg_vec_ptr  = (*handler)->vector_value;
		dbg_vec_dims = (*handler)->vector_dims;

		/* Prevent idydb_clear_values() (inside idydb_read_at) from freeing the staged vector */
		(*handler)->vector_value = NULL;
		(*handler)->vector_dims  = 0;
	}

	if (dbg_have)
	{
		(void)idydb_dbg_peek_cell_repr(handler, dbg_col, dbg_row, dbg_before, sizeof(dbg_before));

		/* Restore staged payload (idydb_read_at clobbered handler state) */
		idydb_clear_values(handler);
		(*handler)->value_type = dbg_stage_type;
		(*handler)->value_retrieved = false;

		if (dbg_stage_type == IDYDB_INTEGER) (*handler)->value.int_value = dbg_i;
		else if (dbg_stage_type == IDYDB_FLOAT) (*handler)->value.float_value = dbg_f;
		else if (dbg_stage_type == IDYDB_BOOL) (*handler)->value.bool_value = dbg_b;
		else if (dbg_stage_type == IDYDB_CHAR && dbg_sdup)
		{
			strncpy((*handler)->value.char_value, dbg_sdup, sizeof((*handler)->value.char_value));
			(*handler)->value.char_value[sizeof((*handler)->value.char_value) - 1] = '\0';
			free(dbg_sdup);
			dbg_sdup = NULL;
		}
		else if (dbg_stage_type == IDYDB_VECTOR)
		{
			(*handler)->vector_value = dbg_vec_ptr;
			(*handler)->vector_dims  = dbg_vec_dims;
		}

		idydb_dbg_format_value_from_handler(*handler, dbg_after, sizeof(dbg_after));
	}
	else
	{
		/* can't peek safely; still log "after" */
		idydb_dbg_format_value_from_handler(*handler, dbg_after, sizeof(dbg_after));
		snprintf(dbg_before, sizeof(dbg_before), "<peek skipped: OOM>");

		if (dbg_sdup) free(dbg_sdup);

		if (dbg_stage_type == IDYDB_VECTOR) {
			(*handler)->vector_value = dbg_vec_ptr;
			(*handler)->vector_dims  = dbg_vec_dims;
		}
	}
#endif

	row_position -= 1;

	unsigned short input_size = 0;
	switch ((*handler)->value_type)
	{
	case IDYDB_INTEGER:
		input_size = sizeof(int);
		break;
	case IDYDB_FLOAT:
		input_size = sizeof(float);
		break;
	case IDYDB_BOOL:
		break;
	case IDYDB_CHAR:
	{
		/* Payload includes '\0' (empty string is represented by one NUL byte). */
		unsigned short len = (unsigned short)strlen((*handler)->value.char_value);
		input_size = (unsigned short)(len + 1); /* payload bytes */
		break;
	}
	case IDYDB_VECTOR:
		if ((*handler)->vector_dims == 0 || (*handler)->vector_dims > IDYDB_MAX_VECTOR_DIM) {
			idydb_clear_values(handler);
			idydb_error_state(handler, 11);
			return IDYDB_ERROR;
		}
		input_size = (unsigned short)((*handler)->vector_dims * sizeof(float));
		break;
	}

	const unsigned short input_size_default = input_size; /* payload bytes */
	if ((*handler)->value_type == IDYDB_CHAR || (*handler)->value_type == IDYDB_VECTOR)
		input_size += sizeof(short);

	idydb_sizing_max offset[6] = {0, 0, 0, 0, 0, 0};
	idydb_size_selection_type skip_offset[2] = {0, 0};
	unsigned short skip_amount[2] = {0, 0};
	unsigned short read_length[2] = {IDYDB_PARTITION_AND_SEGMENT, IDYDB_PARTITION_AND_SEGMENT};
	unsigned short row_count[3] = {0, 0, 0};
	unsigned short current_length[2] = {0, 0};
	unsigned char current_type = 0;

	/* ---------------- READ STRUCTURE (original) ---------------- */
	if ((*handler)->size > 0)
	{
		for (;;)
		{
			read_length[1] = read_length[0];
			offset[2] = offset[0];
			if (offset[0] >= (*handler)->size)
			{
				if ((offset[0] - current_length[0]) > (*handler)->size)
				{
					idydb_clear_values(handler);
					idydb_error_state(handler, 13);
					return IDYDB_CORRUPT;
				}
				else if (skip_offset[0] == column_position)
				{
					offset[0] = offset[3];
					offset[1] = (*handler)->size;
					row_count[0] = row_count[1];
					current_length[0] = 0;
				}
				else if (skip_offset[0] < column_position)
				{
					offset[0] = (*handler)->size;
					offset[1] = offset[0];
					row_count[0] = 0;
					current_length[0] = 0;
				}
				break;
			}
			fseek((*handler)->file_descriptor, offset[0], SEEK_SET);
			offset[0] += read_length[0];
			unsigned char skip_read_offset = 1;
			if (row_count[0] == 0)
			{
				offset[1] = offset[2];
				offset[5] = offset[3];
				offset[3] = offset[2];
				offset[4] = (offset[2] + IDYDB_PARTITION_SIZE);
				skip_amount[1] = skip_amount[0];
				if (fread(&skip_amount[0], 1, sizeof(short), (*handler)->file_descriptor) != sizeof(short))
				{
					idydb_clear_values(handler);
					idydb_error_state(handler, 14);
					return IDYDB_ERROR;
				}
				skip_offset[1] = skip_offset[0];
				skip_offset[0] += skip_amount[0];
				if (skip_offset[0] > IDYDB_COLUMN_POSITION_MAX)
				{
#ifdef IDYDB_ALLOW_UNSAFE
					if (!(*handler)->unsafe)
					{
#endif
						idydb_clear_values(handler);
						idydb_error_state(handler, 22);
						return IDYDB_RANGE;
#ifdef IDYDB_ALLOW_UNSAFE
					}
#endif
				}
				skip_offset[0] += 1;
				if (fread(&row_count[0], 1, sizeof(short), (*handler)->file_descriptor) != sizeof(short))
				{
					idydb_clear_values(handler);
					idydb_error_state(handler, 14);
					return IDYDB_ERROR;
				}
#if IDYDB_SIZING_MODE != IDYDB_SIZING_MODE_BIG
				if (row_count[0] > IDYDB_ROW_POSITION_MAX)
				{
					idydb_clear_values(handler);
					idydb_error_state(handler, 22);
					return IDYDB_RANGE;
				}
#endif
				row_count[2] = row_count[1];
				row_count[0] += 1;
				row_count[1] = row_count[0];
				if (row_count[0] > 1)
					read_length[0] = IDYDB_SEGMENT_SIZE;
			}
			else
			{
				if (skip_offset[0] != column_position)
					fseek((*handler)->file_descriptor, skip_read_offset++, SEEK_CUR);
				offset[1] += read_length[0];
			}
			current_length[0] = 0;
			if (skip_offset[0] == column_position)
			{
				skip_offset[1] = skip_offset[0];
				unsigned short position;
				if (fread(&position, 1, sizeof(short), (*handler)->file_descriptor) != sizeof(short))
				{
					idydb_clear_values(handler);
					idydb_error_state(handler, 14);
					return IDYDB_ERROR;
				}
				if (position == row_position)
				{
					if (fread(&current_type, 1, 1, (*handler)->file_descriptor) != 1)
					{
						idydb_clear_values(handler);
						idydb_error_state(handler, 14);
						return IDYDB_ERROR;
					}
					current_length[1] = 1;
					switch (current_type)
					{
					case IDYDB_READ_INT:   current_length[0] = sizeof(int); break;
					case IDYDB_READ_FLOAT: current_length[0] = sizeof(float); break;
					case IDYDB_READ_CHAR:
						if (fread(&current_length[0], 1, sizeof(short), (*handler)->file_descriptor) != sizeof(short))
						{
							idydb_error_state(handler, 14);
							return IDYDB_ERROR;
						}
						current_length[0] += (1 + sizeof(short));
						break;
					case IDYDB_READ_BOOL_TRUE:
					case IDYDB_READ_BOOL_FALSE:
						break;
					case IDYDB_READ_VECTOR:
					{
						unsigned short dims = 0;
						if (fread(&dims, 1, sizeof(short), (*handler)->file_descriptor) != sizeof(short))
						{
							idydb_error_state(handler, 14);
							return IDYDB_ERROR;
						}
						if (dims == 0 || dims > IDYDB_MAX_VECTOR_DIM)
						{
							idydb_error_state(handler, 22);
							return IDYDB_RANGE;
						}
						current_length[0] = (unsigned short)(sizeof(short) + (dims * sizeof(float)));
						break;
					}
					default:
						idydb_clear_values(handler);
						idydb_error_state(handler, 20);
						return IDYDB_ERROR;
					}
					offset[0] = offset[3];
					offset[1] = offset[2];
					row_count[0] = row_count[1];
					break;
				}
#if IDYDB_SIZING_MODE != IDYDB_SIZING_MODE_BIG
				else if (position > IDYDB_ROW_POSITION_MAX)
				{
					idydb_clear_values(handler);
					idydb_error_state(handler, 22);
					return IDYDB_RANGE;
				}
#endif
				else if (row_count[0] >= 1 && position > row_position)
				{
					offset[0] = offset[3];
					offset[1] = offset[2];
					current_length[0] = 0;
					row_count[0] = row_count[1];
					break;
				}
				else if (row_position < position)
				{
					offset[0] = offset[2];
					current_length[0] = 0;
					row_count[0] = row_count[1];
					break;
				}
			}
			else if (skip_offset[0] > column_position)
			{
				skip_offset[0] = skip_offset[1];
				skip_amount[0] = skip_amount[1];
				offset[1] = offset[2];
				if (skip_offset[0] == column_position)
					offset[0] = offset[5];
				else
					offset[0] = offset[2];
				read_length[0] = read_length[1];
				current_length[0] = 0;
				if (skip_offset[1] == column_position)
					row_count[0] = row_count[2];
				else
					row_count[0] = 0;
				break;
			}
			else
				fseek((*handler)->file_descriptor, (3 - skip_read_offset), SEEK_CUR);

			if (fread(&current_type, 1, 1, (*handler)->file_descriptor) != 1)
			{
				idydb_clear_values(handler);
				idydb_error_state(handler, 14);
				return IDYDB_ERROR;
			}
			switch (current_type)
			{
			case IDYDB_READ_INT: current_length[0] = sizeof(int); break;
			case IDYDB_READ_FLOAT: current_length[0] = sizeof(float); break;
			case IDYDB_READ_CHAR:
				if (fread(&current_length[0], 1, sizeof(short), (*handler)->file_descriptor) != sizeof(short))
				{
					idydb_clear_values(handler);
					idydb_error_state(handler, 14);
					return IDYDB_ERROR;
				}
				current_length[0] += (1 + sizeof(short));
				break;
			case IDYDB_READ_BOOL_TRUE:
			case IDYDB_READ_BOOL_FALSE:
				break;
			case IDYDB_READ_VECTOR:
			{
				unsigned short dims = 0;
				if (fread(&dims, 1, sizeof(short), (*handler)->file_descriptor) != sizeof(short))
				{
					idydb_clear_values(handler);
					idydb_error_state(handler, 14);
					return IDYDB_ERROR;
				}
				current_length[0] = (unsigned short)(sizeof(short) + (dims * sizeof(float)));
				break;
			}
			default:
				idydb_clear_values(handler);
				idydb_error_state(handler, 20);
				return IDYDB_ERROR;
			}
			current_type = 0;
			if (row_count[0] == 1)
				read_length[0] = IDYDB_PARTITION_AND_SEGMENT;
			row_count[0] -= 1;
			offset[0] += current_length[0];
		}
	}

	if (current_length[1] == 0 && (input_size == 0 && (*handler)->value_type == IDYDB_NULL))
		return IDYDB_DONE;

	struct relinquish_excersion { unsigned short size; idydb_sizing_max position; bool use; };

	struct relinquish_excersion info_skip_offset = {0, 0, false};

	struct relinquish_excersion info_row_count = {
		(unsigned short)(row_count[0]),
		0,
		false
	};

	struct relinquish_excersion info_row_position = {(unsigned short)row_position, 0, false};

	/* FIX: CHAR stores strlen (payload-1); VECTOR stores dims */
	unsigned short char_len_field = 0;
	if ((*handler)->value_type == IDYDB_CHAR && input_size_default > 0) char_len_field = (unsigned short)(input_size_default - 1);

	struct relinquish_excersion info_input_size = {
		(unsigned short)(((*handler)->value_type == IDYDB_CHAR) ? char_len_field :
		                 ((*handler)->value_type == IDYDB_VECTOR ? (*handler)->vector_dims : 0)),
		0,
		false
	};

	struct relinquish_excersion info_input_type = {(unsigned short)(*handler)->value_type, 0, false};
	struct relinquish_excersion info_input_buffer = {0, 0, false};

	bool removal = false;

	if (input_size > current_length[0] || (current_length[1] == 0 && (*handler)->value_type != IDYDB_NULL))
	{
		unsigned short offset_sizing = (unsigned short)(input_size - current_length[0]);
		unsigned char additional_offset = 0;
		if (current_length[1] == 0)
		{
			if (row_count[0] == 0) additional_offset = IDYDB_PARTITION_AND_SEGMENT;
			else additional_offset = IDYDB_SEGMENT_SIZE;
		}
		if (offset[1] < (*handler)->size)
		{
			idydb_sizing_max buffer_delimitation_point = (offset[1]);
			idydb_sizing_max buffer_offset = (((*handler)->size - offset[1]) % IDYDB_MAX_BUFFER_SIZE);
			if (buffer_offset == 0) buffer_offset = IDYDB_MAX_BUFFER_SIZE;
			unsigned short buffer_size = (unsigned short)buffer_offset;
			for (;;)
			{
				memset((*handler)->buffer, 0, (sizeof(char) * IDYDB_MAX_BUFFER_SIZE));
				fseek((*handler)->file_descriptor, ((*handler)->size - buffer_offset), SEEK_SET);
				if (fread((*handler)->buffer, buffer_size, 1, (*handler)->file_descriptor) != 1)
				{
					idydb_clear_values(handler);
					idydb_error_state(handler, 14);
					return IDYDB_ERROR;
				}
				fseek((*handler)->file_descriptor, (((*handler)->size - buffer_offset) + offset_sizing + additional_offset), SEEK_SET);
				if (fwrite((*handler)->buffer, buffer_size, 1, (*handler)->file_descriptor) != 1)
				{
					idydb_clear_values(handler);
					idydb_error_state(handler, 15);
					return IDYDB_ERROR;
				}
				if (((*handler)->size - buffer_offset) <= buffer_delimitation_point) break;
				buffer_size = IDYDB_MAX_BUFFER_SIZE;
				buffer_offset += buffer_size;
			}
			fseek((*handler)->file_descriptor, (((*handler)->size - buffer_offset) + offset_sizing + additional_offset), SEEK_SET);
		}
		(*handler)->size += offset_sizing;
		if (current_length[1] == 0) (*handler)->size += additional_offset;
		else row_count[0] -= 1;
	}
	else if (input_size < current_length[0] || (*handler)->value_type == IDYDB_NULL)
	{
		unsigned short offset_sizing = (unsigned short)(current_length[0] - (current_length[0] - input_size));
		if (row_count[0] == 1)
		{
			offset[3] = offset[1];
			offset[1] = offset[4];
		}
		idydb_sizing_max deletion_point[2] = {
			(offset[1] + IDYDB_SEGMENT_SIZE + current_length[0]),
			(offset[1] + IDYDB_SEGMENT_SIZE + offset_sizing),
		};
		if (offset[0] == offset[1]) deletion_point[0] += IDYDB_PARTITION_SIZE;

		if (input_size == 0 && (*handler)->value_type == IDYDB_NULL)
		{
			if (row_count[0] > 1)
			{
				if (offset[0] == offset[1])
				{
					deletion_point[1] -= (((deletion_point[1] - offset[0])) );
					deletion_point[1] += IDYDB_PARTITION_SIZE;
				}
				else
					deletion_point[1] -= (IDYDB_SEGMENT_SIZE);
			}
			else if (offset[3] == offset[0])
			{
				if (offset[0] == offset[1])
				{
					deletion_point[0] += IDYDB_PARTITION_SIZE;
					deletion_point[1] += IDYDB_PARTITION_SIZE;
				}
				else if (row_count[0] == 1)
					deletion_point[1] -= IDYDB_PARTITION_AND_SEGMENT;
				else
					deletion_point[1] -= IDYDB_PARTITION_SIZE;
			}
			else
				deletion_point[1] -= (deletion_point[1] - (deletion_point[1] - offset[1]));
		}
		else if (offset[0] == offset[1])
			deletion_point[1] += IDYDB_PARTITION_SIZE;

		unsigned short buffer_size = IDYDB_MAX_BUFFER_SIZE;
		idydb_sizing_max buffer_offset = 0;
		bool writable = (deletion_point[0] != (*handler)->size);
		while (writable)
		{
			if ((deletion_point[0] + buffer_offset + buffer_size) >= (*handler)->size)
			{
				buffer_size = (unsigned short)((*handler)->size - (deletion_point[0] + buffer_offset));
				writable = false;
				if (buffer_size == 0) break;
			}
			memset((*handler)->buffer, 0, (sizeof(char) * IDYDB_MAX_BUFFER_SIZE));
			fseek((*handler)->file_descriptor, (deletion_point[0] + buffer_offset), SEEK_SET);
			if (fread((*handler)->buffer, buffer_size, 1, (*handler)->file_descriptor) != 1)
			{
				idydb_clear_values(handler);
				idydb_error_state(handler, 14);
				return IDYDB_ERROR;
			}
			fseek((*handler)->file_descriptor, (deletion_point[1] + buffer_offset), SEEK_SET);
			if (fwrite((*handler)->buffer, buffer_size, 1, (*handler)->file_descriptor) != 1)
			{
				idydb_clear_values(handler);
				idydb_error_state(handler, 15);
				return IDYDB_ERROR;
			}
			buffer_offset += buffer_size;
		}
		(*handler)->size -= (current_length[0] - offset_sizing);
		if (input_size == 0 && (*handler)->value_type == IDYDB_NULL)
		{
			if (row_count[0] > 1) (*handler)->size -= IDYDB_SEGMENT_SIZE;
			else (*handler)->size -= IDYDB_PARTITION_AND_SEGMENT;
		}
		if (ftruncate(fileno((*handler)->file_descriptor), (*handler)->size) != 0)
		{
			idydb_clear_values(handler);
			idydb_error_state(handler, 17);
			return IDYDB_CORRUPT;
		}
		row_count[0] -= 1;
		if (row_count[0] == 0 && input_size == 0 && (*handler)->value_type == IDYDB_NULL)
			removal = true;
	}
	else
		row_count[0] -= 1;

	if (offset[0] == offset[1]) offset[1] += IDYDB_PARTITION_SIZE;

	info_skip_offset.position = offset[0];
	info_row_count.position = (offset[0] + 2);
	info_row_count.size = row_count[0];
	if ((*handler)->value_type == IDYDB_NULL) info_row_count.size -= 1;

	if (((input_size == 0 && (*handler)->value_type != IDYDB_NULL) ? true : (input_size != 0)) && !removal)
	{
		info_skip_offset.use = true;
		if (row_count[0] == 0)
		{
			if (current_length[0] != 0 || current_length[1] == 1)
				info_skip_offset.use = false;
			else if (offset[0] != 0)
				skip_amount[0] = (unsigned short)(column_position - (skip_offset[0] + 1));
			else
				skip_amount[0] = (unsigned short)(column_position - 1);
		}
		info_skip_offset.size = skip_amount[0];

		info_row_position.use = true;
		info_row_position.position = (offset[1]);

		info_input_type.use = true;
		info_input_type.position = (offset[1] + 2);

		if ((*handler)->value_type == IDYDB_CHAR || (*handler)->value_type == IDYDB_VECTOR)
		{
			info_input_size.use = true;
			info_input_size.position = (offset[1] + 3);
			info_input_buffer.position = (offset[1] + 5);
		}
		else
		{
			info_input_buffer.position = (offset[1] + 3);
		}

		info_input_buffer.use = ((*handler)->value_type != IDYDB_BOOL);
		info_row_count.use = true;

		if (current_length[0] == 0 && current_length[1] == 0 && row_count[0] == 0 && (current_length[0] != input_size || (*handler)->value_type == IDYDB_BOOL))
		{
			offset[0] += (IDYDB_PARTITION_AND_SEGMENT + input_size);
			if (offset[0] != (*handler)->size)
			{
				if (fread(&skip_amount[1], sizeof(short), 1, (*handler)->file_descriptor) != 1)
				{
					idydb_clear_values(handler);
					idydb_error_state(handler, 14);
					return IDYDB_ERROR;
				}
				if (skip_amount[1] == 1) skip_amount[0] = 0;
				else skip_amount[0] = (unsigned short)(skip_amount[1] - (skip_amount[0] + 1));
				fseek((*handler)->file_descriptor, offset[0], SEEK_SET);
				if (fwrite(&skip_amount[0], sizeof(short), 1, (*handler)->file_descriptor) != 1)
				{
					idydb_clear_values(handler);
					idydb_error_state(handler, 16);
					return IDYDB_ERROR;
				}
			}
		}
	}
	else if (offset[0] != (*handler)->size)
	{
		if (row_count[0] == 0)
		{
			info_skip_offset.use = true;
			fseek((*handler)->file_descriptor, offset[0], SEEK_SET);
			skip_amount[1] = skip_amount[0];
			if (fread(&skip_amount[0], 1, sizeof(short), (*handler)->file_descriptor) != sizeof(short))
			{
				idydb_clear_values(handler);
				idydb_error_state(handler, 14);
				return IDYDB_ERROR;
			}
			skip_amount[0] += (unsigned short)(skip_amount[1] + 1);
			info_skip_offset.size = skip_amount[0];
		}
		else
			info_row_count.use = true;
	}

	if (info_skip_offset.use)
	{
		fseek((*handler)->file_descriptor, info_skip_offset.position, SEEK_SET);
		if (fwrite(&info_skip_offset.size, sizeof(short), 1, (*handler)->file_descriptor) != 1)
		{
			idydb_clear_values(handler);
			idydb_error_state(handler, 15);
			return IDYDB_ERROR;
		}
	}
	if (info_row_count.use)
	{
		fseek((*handler)->file_descriptor, info_row_count.position, SEEK_SET);
		if (fwrite(&info_row_count.size, sizeof(short), 1, (*handler)->file_descriptor) != 1)
		{
			idydb_clear_values(handler);
			idydb_error_state(handler, 15);
			return IDYDB_ERROR;
		}
	}
	if (info_row_position.use)
	{
		fseek((*handler)->file_descriptor, info_row_position.position, SEEK_SET);
		if (fwrite(&info_row_position.size, sizeof(short), 1, (*handler)->file_descriptor) != 1)
		{
			idydb_clear_values(handler);
			idydb_error_state(handler, 15);
			return IDYDB_ERROR;
		}
	}
	if (info_input_size.use)
	{
		fseek((*handler)->file_descriptor, info_input_size.position, SEEK_SET);
		if (fwrite(&info_input_size.size, sizeof(short), 1, (*handler)->file_descriptor) != 1)
		{
			idydb_clear_values(handler);
			idydb_error_state(handler, 15);
			return IDYDB_ERROR;
		}
	}
	if (info_input_type.use)
	{
		fseek((*handler)->file_descriptor, info_input_type.position, SEEK_SET);
		unsigned short input_type = 0;
		switch (info_input_type.size)
		{
		case IDYDB_INTEGER: input_type = IDYDB_READ_INT; break;
		case IDYDB_FLOAT:   input_type = IDYDB_READ_FLOAT; break;
		case IDYDB_CHAR:    input_type = IDYDB_READ_CHAR; break;
		case IDYDB_BOOL:    input_type = ((*handler)->value.bool_value ? IDYDB_READ_BOOL_TRUE : IDYDB_READ_BOOL_FALSE); break;
		case IDYDB_VECTOR:  input_type = IDYDB_READ_VECTOR; break;
		}
		if (fwrite(&input_type, 1, 1, (*handler)->file_descriptor) != 1)
		{
			idydb_clear_values(handler);
			idydb_error_state(handler, 15);
			return IDYDB_ERROR;
		}
	}
	if (info_input_buffer.use)
	{
		fseek((*handler)->file_descriptor, info_input_buffer.position, SEEK_SET);
		switch (info_input_type.size)
		{
		case IDYDB_INTEGER:
			if (fwrite(&(*handler)->value.int_value, sizeof(int), 1, (*handler)->file_descriptor) != 1)
			{
				idydb_clear_values(handler);
				idydb_error_state(handler, 15);
				return IDYDB_ERROR;
			}
			break;
		case IDYDB_FLOAT:
			if (fwrite(&(*handler)->value.float_value, sizeof(float), 1, (*handler)->file_descriptor) != 1)
			{
				idydb_clear_values(handler);
				idydb_error_state(handler, 15);
				return IDYDB_ERROR;
			}
			break;
		case IDYDB_CHAR:
			/* FIX: write payload including '\0' */
			if (fwrite((*handler)->value.char_value, 1, input_size_default, (*handler)->file_descriptor) != input_size_default)
			{
				idydb_clear_values(handler);
				idydb_error_state(handler, 15);
				return IDYDB_ERROR;
			}
			break;
		case IDYDB_VECTOR:
			if (fwrite((*handler)->vector_value, sizeof(float), (*handler)->vector_dims, (*handler)->file_descriptor) != (*handler)->vector_dims)
			{
				idydb_clear_values(handler);
				idydb_error_state(handler, 15);
				return IDYDB_ERROR;
			}
			break;
		}
	}

	/* capture staged type before clear_values resets it */
#if CUWACUNU_CAMAHJUCUNU_DB_VERBOSE_DEBUG
	idydb_sizing_max dbg_size_after = (*handler)->size;
	long long dbg_delta = (long long)dbg_size_after - (long long)dbg_size_before;

	const char* dbg_op = "UPDATE";
	if (strcmp(dbg_before, dbg_after) == 0) dbg_op = "NOOP";
	else if (strncmp(dbg_before, "NULL", 4) == 0 && strncmp(dbg_after, "NULL", 4) != 0) dbg_op = "INSERT";
	else if (strncmp(dbg_before, "NULL", 4) != 0 && strncmp(dbg_after, "NULL", 4) == 0) dbg_op = "DELETE";
#endif

	idydb_clear_values(handler);

	(*handler)->dirty = true;

#if CUWACUNU_CAMAHJUCUNU_DB_VERBOSE_DEBUG
	DB_DEBUGF(handler,
	          "cell(%llu,%llu) %s: %s -> %s (Δ%+lldB, size=%llu)",
	          (unsigned long long)dbg_col,
	          (unsigned long long)dbg_row,
	          dbg_op,
	          dbg_before,
	          dbg_after,
	          dbg_delta,
	          (unsigned long long)dbg_size_after);
#else
	DB_DEBUG(handler, "database mutated");
#endif

	return IDYDB_DONE;
}

/* ---------------- Vector math helpers ---------------- */

static inline float idydb_dot(const float* a, const float* b, unsigned short d) {
	float s = 0.0f;
	for (unsigned short i = 0; i < d; ++i) s += a[i] * b[i];
	return s;
}
static inline float idydb_norm(const float* a, unsigned short d) {
	return sqrtf(idydb_dot(a, a, d));
}

/* ---------------- Filter helpers ---------------- */

