void idydb_free(void* p) { if (p) free(p); }

void idydb_value_free(idydb_value* v)
{
	if (!v) return;
	if (v->type == IDYDB_CHAR) {
		if (v->as.s) free(v->as.s);
		v->as.s = NULL;
	} else if (v->type == IDYDB_VECTOR) {
		if (v->as.vec.v) free(v->as.vec.v);
		v->as.vec.v = NULL;
		v->as.vec.dims = 0;
	}
	v->type = IDYDB_NULL;
}

void idydb_values_free(idydb_value* values, size_t count)
{
	if (!values) return;
	for (size_t i = 0; i < count; ++i) idydb_value_free(&values[i]);
}

static inline int idydb_filter_cmp_int(int a, idydb_filter_op op, int b)
{
	switch (op) {
		case IDYDB_FILTER_OP_EQ:  return a == b;
		case IDYDB_FILTER_OP_NEQ: return a != b;
		case IDYDB_FILTER_OP_GT:  return a >  b;
		case IDYDB_FILTER_OP_GTE: return a >= b;
		case IDYDB_FILTER_OP_LT:  return a <  b;
		case IDYDB_FILTER_OP_LTE: return a <= b;
		default: return 0;
	}
}

static inline int idydb_filter_cmp_float(float a, idydb_filter_op op, float b)
{
	switch (op) {
		case IDYDB_FILTER_OP_EQ:  return a == b;
		case IDYDB_FILTER_OP_NEQ: return a != b;
		case IDYDB_FILTER_OP_GT:  return a >  b;
		case IDYDB_FILTER_OP_GTE: return a >= b;
		case IDYDB_FILTER_OP_LT:  return a <  b;
		case IDYDB_FILTER_OP_LTE: return a <= b;
		default: return 0;
	}
}

static inline int idydb_filter_cmp_bool(bool a, idydb_filter_op op, bool b)
{
	switch (op) {
		case IDYDB_FILTER_OP_EQ:  return a == b;
		case IDYDB_FILTER_OP_NEQ: return a != b;
		default: return 0;
	}
}

static int idydb_filter_build_term_mask(idydb **handler,
                                       const idydb_filter_term* term,
                                       unsigned char* term_mask,
                                       size_t mask_len)
{
	if (!handler || !*handler || !(*handler)->configured || !term || !term_mask || mask_len == 0) return 0;
	if (term->column == 0) return 0;

#ifdef IDYDB_ALLOW_UNSAFE
	if (!(*handler)->unsafe)
	{
#endif
		if ((term->column - 1) > IDYDB_COLUMN_POSITION_MAX) return 0;
#ifdef IDYDB_ALLOW_UNSAFE
	}
#endif

	/* Normalize NULL equality */
	idydb_filter_op op = term->op;
	unsigned char want_type = term->type;
	if (op == IDYDB_FILTER_OP_EQ  && want_type == IDYDB_NULL) op = IDYDB_FILTER_OP_IS_NULL;
	if (op == IDYDB_FILTER_OP_NEQ && want_type == IDYDB_NULL) op = IDYDB_FILTER_OP_IS_NOT_NULL;

	if (op == IDYDB_FILTER_OP_IS_NULL) memset(term_mask, 1, mask_len);
	else memset(term_mask, 0, mask_len);
	if (mask_len > 0) term_mask[0] = 0;

	idydb_sizing_max offset = 0;
	idydb_size_selection_type skip_offset = 0;
	unsigned char read_length = IDYDB_PARTITION_AND_SEGMENT;
	unsigned short row_count = 0;

	while ((offset + read_length) <= (*handler)->size)
	{
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
			unsigned short skip_amount = 0;
#ifdef IDYDB_MMAP_OK
			if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
				skip_amount = (unsigned short)idydb_read_mmap(offset, sizeof(short), (*handler)->buffer).integer;
			else
#endif
				if (fread(&skip_amount, 1, sizeof(short), (*handler)->file_descriptor) != sizeof(short)) return 0;

			skip_offset += skip_amount;
			skip_offset += 1;

#ifdef IDYDB_MMAP_OK
			if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
				row_count = (unsigned short)idydb_read_mmap((offset + sizeof(short)), sizeof(short), (*handler)->buffer).integer;
			else
#endif
				if (fread(&row_count, 1, sizeof(short), (*handler)->file_descriptor) != sizeof(short)) return 0;
			row_count += 1;
		}

		unsigned char set_read_length = IDYDB_PARTITION_AND_SEGMENT;
		const bool in_target = (skip_offset == term->column);
		unsigned short row_pos = 0;

		if (in_target)
		{
#ifdef IDYDB_MMAP_OK
			if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
				row_pos = (unsigned short)idydb_read_mmap((offset + ((sizeof(short) * 2) * (read_length == IDYDB_PARTITION_AND_SEGMENT))), sizeof(short), (*handler)->buffer).integer;
			else
#endif
				if (fread(&row_pos, 1, sizeof(short), (*handler)->file_descriptor) != sizeof(short)) return 0;
		}
		else
		{
#ifdef IDYDB_MMAP_OK
			if ((*handler)->read_only != IDYDB_READONLY_MMAPPED)
#endif
				fseek((*handler)->file_descriptor, sizeof(short), SEEK_CUR);
		}

		if (row_count > 1) { row_count -= 1; set_read_length = IDYDB_SEGMENT_SIZE; }

		unsigned char data_type;
#ifdef IDYDB_MMAP_OK
		idydb_sizing_max offset_mmap_standard_diff = offset + (sizeof(short) * 3);
		if (read_length == IDYDB_SEGMENT_SIZE) offset_mmap_standard_diff = (offset + sizeof(short));

		if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
		{
			if (read_length == IDYDB_PARTITION_AND_SEGMENT)
				data_type = (unsigned short)idydb_read_mmap((offset + (sizeof(short) * 3)), 1, (*handler)->buffer).integer;
			else
				data_type = (unsigned short)idydb_read_mmap((offset + sizeof(short)), 1, (*handler)->buffer).integer;
			offset_mmap_standard_diff += 1; /* now points to payload start */
		}
		else
#endif
			if (fread(&data_type, 1, 1, (*handler)->file_descriptor) != 1) return 0;

		unsigned short adv = 0;

		/* Helper: set mask for this row if in range */
		const idydb_column_row_sizing row_api = (idydb_column_row_sizing)(row_pos + 1);
		const int row_in_range = ((size_t)row_api < mask_len);

		switch (data_type)
		{
			case IDYDB_READ_BOOL_TRUE:
			case IDYDB_READ_BOOL_FALSE:
			{
				adv = 0;
				if (in_target && row_in_range)
				{
					if (op == IDYDB_FILTER_OP_IS_NULL) term_mask[row_api] = 0;
					else if (op == IDYDB_FILTER_OP_IS_NOT_NULL) term_mask[row_api] = 1;
					else if (want_type == IDYDB_BOOL) {
						bool v = (data_type == IDYDB_READ_BOOL_TRUE);
						term_mask[row_api] = (unsigned char)(idydb_filter_cmp_bool(v, op, term->value.b) ? 1 : 0);
					}
				}
				break;
			}

			case IDYDB_READ_INT:
			{
				adv = sizeof(int);
				if (in_target && row_in_range)
				{
					if (op == IDYDB_FILTER_OP_IS_NULL) term_mask[row_api] = 0;
					else if (op == IDYDB_FILTER_OP_IS_NOT_NULL) term_mask[row_api] = 1;
					else if (want_type == IDYDB_INTEGER) {
						int v = 0;
#ifdef IDYDB_MMAP_OK
						if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
							v = idydb_read_mmap(offset_mmap_standard_diff, sizeof(int), (*handler)->buffer).integer;
						else
#endif
						{
							if (fread(&v, sizeof(int), 1, (*handler)->file_descriptor) != 1) return 0;
						}
						term_mask[row_api] = (unsigned char)(idydb_filter_cmp_int(v, op, term->value.i) ? 1 : 0);
					}
				}
				break;
			}

			case IDYDB_READ_FLOAT:
			{
				adv = sizeof(float);
				if (in_target && row_in_range)
				{
					if (op == IDYDB_FILTER_OP_IS_NULL) term_mask[row_api] = 0;
					else if (op == IDYDB_FILTER_OP_IS_NOT_NULL) term_mask[row_api] = 1;
					else if (want_type == IDYDB_FLOAT) {
						float v = 0.0f;
#ifdef IDYDB_MMAP_OK
						if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
							v = idydb_read_mmap(offset_mmap_standard_diff, sizeof(float), (*handler)->buffer).floating_point;
						else
#endif
						{
							if (fread(&v, sizeof(float), 1, (*handler)->file_descriptor) != 1) return 0;
						}
						term_mask[row_api] = (unsigned char)(idydb_filter_cmp_float(v, op, term->value.f) ? 1 : 0);
					}
				}
				break;
			}

			case IDYDB_READ_CHAR:
			{
				unsigned short n = 0; /* stored length (no '\0') */
#ifdef IDYDB_MMAP_OK
					if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
						n = (unsigned short)idydb_read_mmap(offset_mmap_standard_diff, sizeof(short), (*handler)->buffer).integer;
					else
#endif
					{
						if (fread(&n, sizeof(short), 1, (*handler)->file_descriptor) != 1) return 0;
					}
					adv = (unsigned short)(sizeof(short) + n + 1);

				if (in_target && row_in_range)
				{
					if (op == IDYDB_FILTER_OP_IS_NULL) term_mask[row_api] = 0;
					else if (op == IDYDB_FILTER_OP_IS_NOT_NULL) term_mask[row_api] = 1;
					else if (want_type == IDYDB_CHAR)
					{
						const char* want = (term->value.s ? term->value.s : "");
						const size_t want_len = strlen(want);
						int eq = 0;
						if (want_len == (size_t)n)
						{
#ifdef IDYDB_MMAP_OK
							if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
							{
								const char* p = (const char*)((char*)(*handler)->buffer + (offset_mmap_standard_diff + sizeof(short)));
								eq = (memcmp(p, want, want_len) == 0);
							}
							else
#endif
							{
								/* Stream compare; ok to stop early (next loop fseek()s anyway) */
								size_t pos = 0;
								eq = 1;
								char buf[1024];
								while (pos < want_len)
								{
									size_t chunk = want_len - pos;
									if (chunk > sizeof(buf)) chunk = sizeof(buf);
									if (fread(buf, 1, chunk, (*handler)->file_descriptor) != chunk) { eq = 0; break; }
									if (memcmp(buf, want + pos, chunk) != 0) { eq = 0; break; }
									pos += chunk;
								}
							}
						}
						if (op == IDYDB_FILTER_OP_EQ) term_mask[row_api] = (unsigned char)(eq ? 1 : 0);
						else if (op == IDYDB_FILTER_OP_NEQ) term_mask[row_api] = (unsigned char)(eq ? 0 : 1);
					}
				}
				break;
			}

			case IDYDB_READ_VECTOR:
			{
				unsigned short d = 0;
#ifdef IDYDB_MMAP_OK
					if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
						d = (unsigned short)idydb_read_mmap(offset_mmap_standard_diff, sizeof(short), (*handler)->buffer).integer;
					else
#endif
					{
						if (fread(&d, sizeof(short), 1, (*handler)->file_descriptor) != 1) return 0;
					}
					adv = (unsigned short)(sizeof(short) + d * sizeof(float));

				if (in_target && row_in_range)
				{
					/* Only NULL-ness is supported for VECTOR in filters (keeps it sane). */
					if (op == IDYDB_FILTER_OP_IS_NULL) term_mask[row_api] = 0;
					else if (op == IDYDB_FILTER_OP_IS_NOT_NULL) term_mask[row_api] = 1;
				}
				break;
			}

			default:
				return 0;
		}

#ifdef IDYDB_MMAP_OK
		if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
			offset += read_length;
#endif
		read_length = set_read_length;
		offset += adv;
	}

	return 1;
}

static int idydb_filter_build_allowed_mask(idydb **handler,
                                          const idydb_filter* filter,
                                          unsigned char* allowed,
                                          size_t allowed_len)
{
	if (!allowed || allowed_len == 0) return 0;
	memset(allowed, 1, allowed_len);
	if (allowed_len > 0) allowed[0] = 0;

	if (!filter || filter->nterms == 0 || !filter->terms) return 1;

	unsigned char* tmp = (unsigned char*)malloc(allowed_len);
	if (!tmp) return 0;

	for (size_t t = 0; t < filter->nterms; ++t)
	{
		if (!idydb_filter_build_term_mask(handler, &filter->terms[t], tmp, allowed_len))
		{
			free(tmp);
			return 0;
		}
		for (size_t i = 0; i < allowed_len; ++i)
			allowed[i] = (unsigned char)((allowed[i] && tmp[i]) ? 1 : 0);
	}

	free(tmp);
	return 1;
}

/* ---------------- Column scanning for kNN ---------------- */

static int idydb_knn_search_vector_column_internal(idydb **handler,
                                   idydb_column_row_sizing vector_column,
                                   const float* query,
                                   unsigned short dims,
                                   unsigned short k,
                                   idydb_similarity_metric metric,
                                   const unsigned char* allowed,
                                   size_t allowed_len,
                                   idydb_knn_result* out_results)
{
	if (!handler || !*handler || !(*handler)->configured || !query || dims == 0 || dims > IDYDB_MAX_VECTOR_DIM || k == 0 || !out_results)
	{
		idydb_error_state_if_available(handler, 8);
		return -1;
	}
#ifdef IDYDB_ALLOW_UNSAFE
	if (!(*handler)->unsafe)
	{
#endif
		if (vector_column == 0 || (vector_column - 1) > IDYDB_COLUMN_POSITION_MAX)
		{
			idydb_error_state(handler, 12);
			return -1;
		}
#ifdef IDYDB_ALLOW_UNSAFE
	}
#endif
	for (unsigned short i = 0; i < k; ++i) { out_results[i].row = 0; out_results[i].score = -INFINITY; }

	float query_norm = 1.0f;
	if (metric == IDYDB_SIM_COSINE)
	{
		query_norm = idydb_norm(query, dims);
		if (query_norm == 0.0f) query_norm = 1.0f;
	}

	idydb_sizing_max offset = 0;
	idydb_size_selection_type skip_offset = 0;
	unsigned char read_length = IDYDB_PARTITION_AND_SEGMENT;
	unsigned short row_count = 0;

	while ((offset + read_length) <= (*handler)->size)
	{
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
			unsigned short skip_amount = 0;
#ifdef IDYDB_MMAP_OK
			if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
				skip_amount = (unsigned short)idydb_read_mmap(offset, sizeof(short), (*handler)->buffer).integer;
			else
#endif
				if (fread(&skip_amount, 1, sizeof(short), (*handler)->file_descriptor) != sizeof(short))
			{
				idydb_error_state(handler, 14);
				return -1;
			}
			skip_offset += skip_amount;
			skip_offset += 1;
#ifdef IDYDB_MMAP_OK
			if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
				row_count = (unsigned short)idydb_read_mmap((offset + sizeof(short)), sizeof(short), (*handler)->buffer).integer;
			else
#endif
				if (fread(&row_count, 1, sizeof(short), (*handler)->file_descriptor) != sizeof(short))
			{
				idydb_error_state(handler, 14);
				return -1;
			}
			row_count += 1;
		}

		unsigned char set_read_length = IDYDB_PARTITION_AND_SEGMENT;

		unsigned short row_pos = 0;
		if (skip_offset == vector_column) {
#ifdef IDYDB_MMAP_OK
			if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
				row_pos = (unsigned short)idydb_read_mmap((offset + ((sizeof(short) * 2) * (read_length == IDYDB_PARTITION_AND_SEGMENT))), sizeof(short), (*handler)->buffer).integer;
			else
#endif
				if (fread(&row_pos, 1, sizeof(short), (*handler)->file_descriptor) != sizeof(short))
			{
				idydb_error_state(handler, 14);
				return -1;
			}
		} else {
#ifdef IDYDB_MMAP_OK
			if ((*handler)->read_only != IDYDB_READONLY_MMAPPED)
#endif
				fseek((*handler)->file_descriptor, sizeof(short), SEEK_CUR);
		}

		if (row_count > 1) { row_count -= 1; set_read_length = IDYDB_SEGMENT_SIZE; }

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
			return -1;
		}

		unsigned short adv = 0;
		switch (data_type)
		{
			case IDYDB_READ_CHAR: {
				unsigned short n = 0;
#ifdef IDYDB_MMAP_OK
				if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
					n = (unsigned short)idydb_read_mmap(offset_mmap_standard_diff, sizeof(short), (*handler)->buffer).integer;
				else
#endif
					if (fread(&n, 1, sizeof(short), (*handler)->file_descriptor) != sizeof(short)) { idydb_error_state(handler, 14); return -1; }
				adv = (unsigned short)(sizeof(short) + n + 1);
				break;
			}
			case IDYDB_READ_INT:   adv = sizeof(int);   break;
			case IDYDB_READ_FLOAT: adv = sizeof(float); break;
			case IDYDB_READ_BOOL_TRUE:
			case IDYDB_READ_BOOL_FALSE:
				adv = 0; break;
			case IDYDB_READ_VECTOR: {
				unsigned short vdims = 0;
#ifdef IDYDB_MMAP_OK
				if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
					vdims = (unsigned short)idydb_read_mmap(offset_mmap_standard_diff, sizeof(short), (*handler)->buffer).integer;
				else
#endif
					if (fread(&vdims, 1, sizeof(short), (*handler)->file_descriptor) != sizeof(short)) { idydb_error_state(handler, 14); return -1; }
				adv = (unsigned short)(sizeof(short) + vdims * sizeof(float));

				if (skip_offset == vector_column && vdims == dims) {
					const idydb_column_row_sizing row_api = (idydb_column_row_sizing)(row_pos + 1);
					if (allowed && ((size_t)row_api >= allowed_len || allowed[row_api] == 0))
					{
						/* Row filtered out: do not score it. In non-mmap, we also avoid reading floats. */
#ifdef IDYDB_MMAP_OK
						if ((*handler)->read_only != IDYDB_READONLY_MMAPPED)
#endif
						{
							/* No-op: next loop fseek()s to offset anyway. */
						}
						break;
					}
					float score = -INFINITY;
#ifdef IDYDB_MMAP_OK
					if ((*handler)->read_only == IDYDB_READONLY_MMAPPED) {
						idydb_sizing_max base = offset_mmap_standard_diff + sizeof(short);
						float dot = 0.0f, l2acc = 0.0f, normB = 0.0f;
						for (unsigned short i = 0; i < dims; ++i) {
							union idydb_read_mmap_response f =
								idydb_read_mmap((unsigned int)(base + (i * sizeof(float))), (unsigned char)sizeof(float), (*handler)->buffer);
							float b = f.floating_point;
							if (metric == IDYDB_SIM_COSINE) { dot += query[i] * b; normB += b*b; }
							else { float d = query[i]-b; l2acc += d*d; }
						}
						if (metric == IDYDB_SIM_COSINE) {
							normB = sqrtf(normB); if (normB == 0.0f) normB = 1.0f;
							score = dot / (query_norm * normB);
						} else {
							score = -sqrtf(l2acc);
						}
					} else
#endif
					{
						float dot = 0.0f, l2acc = 0.0f, normB = 0.0f;
						for (unsigned short i = 0; i < dims; ++i) {
							float b;
							if (fread(&b, sizeof(float), 1, (*handler)->file_descriptor) != 1) { idydb_error_state(handler, 18); return -1; }
							if (metric == IDYDB_SIM_COSINE) { dot += query[i]*b; normB += b*b; }
							else { float d = query[i]-b; l2acc += d*d; }
						}
						if (metric == IDYDB_SIM_COSINE) {
							float normBv = sqrtf(normB); if (normBv == 0.0f) normBv = 1.0f;
							score = dot / (query_norm * normBv);
						} else {
							score = -sqrtf(l2acc);
						}
					}

					unsigned short worst = 0;
					float worstScore = out_results[0].score;
					for (unsigned short i = 1; i < k; ++i) {
						if (out_results[i].score < worstScore) { worstScore = out_results[i].score; worst = i; }
					}
					if (score > worstScore) {
						out_results[worst].row = (idydb_column_row_sizing)(row_pos + 1);
						out_results[worst].score = score;
					}
				}
				break;
			}
			default:
				idydb_error_state(handler, 20);
				return -1;
		}

#ifdef IDYDB_MMAP_OK
		if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
			offset += read_length;
#endif
		read_length = set_read_length;
		offset += adv;
	}

	for (unsigned short i = 0; i < k; ++i) {
		for (unsigned short j = i+1; j < k; ++j) {
			if (out_results[j].row != 0 && (out_results[i].row == 0 || out_results[j].score > out_results[i].score)) {
				idydb_knn_result tmp = out_results[i];
				out_results[i] = out_results[j];
				out_results[j] = tmp;
			}
		}
	}

	unsigned short count = 0;
	for (unsigned short i = 0; i < k; ++i) if (out_results[i].row != 0) ++count;
	return (int)count;
}

int idydb_knn_search_vector_column(idydb **handler,
                                   idydb_column_row_sizing vector_column,
                                   const float* query,
                                   unsigned short dims,
                                   unsigned short k,
                                   idydb_similarity_metric metric,
                                   idydb_knn_result* out_results)
{
	return idydb_knn_search_vector_column_internal(handler, vector_column, query, dims, k, metric, NULL, 0, out_results);
}

int idydb_knn_search_vector_column_filtered(idydb **handler,
                                            idydb_column_row_sizing vector_column,
                                            const float* query,
                                            unsigned short dims,
                                            unsigned short k,
                                            idydb_similarity_metric metric,
                                            const idydb_filter* filter,
                                            idydb_knn_result* out_results)
{
	if (!handler || !*handler || !out_results) { idydb_error_state_if_available(handler, 8); return -1; }

	const size_t allowed_len = (size_t)IDYDB_ROW_POSITION_MAX + 2;
	unsigned char* allowed = NULL;

	if (filter && filter->terms && filter->nterms > 0)
	{
		allowed = (unsigned char*)malloc(allowed_len);
		if (!allowed) { idydb_error_state_if_available(handler, 24); return -1; }
		if (!idydb_filter_build_allowed_mask(handler, filter, allowed, allowed_len))
		{
			free(allowed);
			idydb_error_state_if_available(handler, 26);
			return -1;
		}
	}

	int n = idydb_knn_search_vector_column_internal(handler, vector_column, query, dims, k, metric, allowed, allowed_len, out_results);
	if (allowed) free(allowed);
	return n;
}

/* ---------------- Utility: next row index ---------------- */
/* Your original function (unchanged) */

idydb_column_row_sizing idydb_column_next_row(idydb **handler, idydb_column_row_sizing column)
{
	if (!handler || !*handler || !(*handler)->configured) return 1;
	idydb_sizing_max offset = 0;
	idydb_size_selection_type skip_offset = 0;
	unsigned char read_length = IDYDB_PARTITION_AND_SEGMENT;
	unsigned short row_count = 0;
	unsigned short max_row = 0;
	while ((offset + read_length) <= (*handler)->size)
	{
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
			unsigned short skip_amount = 0;
#ifdef IDYDB_MMAP_OK
			if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
				skip_amount = (unsigned short)idydb_read_mmap(offset, sizeof(short), (*handler)->buffer).integer;
			else
#endif
				if (fread(&skip_amount, 1, sizeof(short), (*handler)->file_descriptor) != sizeof(short)) break;
			skip_offset += skip_amount;
			skip_offset += 1;
#ifdef IDYDB_MMAP_OK
			if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
				row_count = (unsigned short)idydb_read_mmap((offset + sizeof(short)), sizeof(short), (*handler)->buffer).integer;
			else
#endif
				if (fread(&row_count, 1, sizeof(short), (*handler)->file_descriptor) != sizeof(short)) break;
			row_count += 1;
		}
		unsigned char set_read_length = IDYDB_PARTITION_AND_SEGMENT;
		if (skip_offset == column)
		{
			unsigned short row_pos = 0;
#ifdef IDYDB_MMAP_OK
			if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
				row_pos = (unsigned short)idydb_read_mmap((offset + ((sizeof(short) * 2) * (read_length == IDYDB_PARTITION_AND_SEGMENT))), sizeof(short), (*handler)->buffer).integer;
			else
#endif
				if (fread(&row_pos, 1, sizeof(short), (*handler)->file_descriptor) != sizeof(short)) break;
			if ((row_pos + 1) > max_row) max_row = (unsigned short)(row_pos + 1);
		}
		else
#ifdef IDYDB_MMAP_OK
			if ((*handler)->read_only != IDYDB_READONLY_MMAPPED)
#endif
			fseek((*handler)->file_descriptor, sizeof(short), SEEK_CUR);

		if (row_count > 1) { row_count -= 1; set_read_length = IDYDB_SEGMENT_SIZE; }

		unsigned char t;
#ifdef IDYDB_MMAP_OK
		idydb_sizing_max offset_mmap_standard_diff = offset + (sizeof(short) * 3);
		if (read_length == IDYDB_SEGMENT_SIZE)
			offset_mmap_standard_diff = (offset + sizeof(short));
		if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
			t = (unsigned short)idydb_read_mmap((read_length == IDYDB_PARTITION_AND_SEGMENT ? (offset + (sizeof(short) * 3)) : (offset + sizeof(short))), 1, (*handler)->buffer).integer;
		else
#endif
			if (fread(&t, 1, 1, (*handler)->file_descriptor) != 1) break;

		unsigned short adv = 0;
		switch (t) {
			case IDYDB_READ_CHAR: {
				unsigned short n = 0;
#ifdef IDYDB_MMAP_OK
				if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
					n = (unsigned short)idydb_read_mmap(offset_mmap_standard_diff, sizeof(short), (*handler)->buffer).integer;
				else
#endif
					if (fread(&n, 1, sizeof(short), (*handler)->file_descriptor) != sizeof(short)) n = 0;
				adv = (unsigned short)(sizeof(short) + n + 1);
				break;
			}
			case IDYDB_READ_INT:   adv = sizeof(int); break;
			case IDYDB_READ_FLOAT: adv = sizeof(float); break;
			case IDYDB_READ_BOOL_TRUE:
			case IDYDB_READ_BOOL_FALSE:
				adv = 0; break;
			case IDYDB_READ_VECTOR: {
				unsigned short d = 0;
#ifdef IDYDB_MMAP_OK
				if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
					d = (unsigned short)idydb_read_mmap(offset_mmap_standard_diff, sizeof(short), (*handler)->buffer).integer;
				else
#endif
					if (fread(&d, 1, sizeof(short), (*handler)->file_descriptor) != sizeof(short)) d = 0;
				adv = (unsigned short)(sizeof(short) + d * sizeof(float));
				break;
			}
			default: break;
		}
#ifdef IDYDB_MMAP_OK
		if ((*handler)->read_only == IDYDB_READONLY_MMAPPED)
			offset += read_length;
#endif
		read_length = set_read_length;
		offset += adv;
	}
	return (idydb_column_row_sizing)(max_row + 1);
}

/* ---------------- RAG helpers (unchanged from your version) ---------------- */

static void idydb_text_results_reset(char** out_texts, unsigned short k)
{
	if (!out_texts) return;
	for (unsigned short i = 0; i < k; ++i) out_texts[i] = NULL;
}

static void idydb_text_results_free(char** out_texts, unsigned short k)
{
	if (!out_texts) return;
	for (unsigned short i = 0; i < k; ++i)
	{
		if (out_texts[i]) free(out_texts[i]);
		out_texts[i] = NULL;
	}
}

void idydb_set_embedder(idydb **handler, idydb_embed_fn fn, void* user) {
	if (!handler || !*handler) return;
	(*handler)->embedder = fn;
	(*handler)->embedder_user = user;
}

int idydb_rag_upsert_text(idydb **handler,
                          idydb_column_row_sizing text_column,
                          idydb_column_row_sizing vector_column,
                          idydb_column_row_sizing row,
                          const char* text,
                          const float* embedding,
                          unsigned short dims)
{
	if (!handler || !*handler || !text || !embedding || dims == 0) {
		idydb_error_state_if_available(handler, 8);
		return IDYDB_ERROR;
	}
	int rc = idydb_insert_const_char(handler, text_column, row, text);
	if (rc != IDYDB_DONE) return rc;
	rc = idydb_insert_vector(handler, vector_column, row, embedding, dims);
	return rc;
}

int idydb_rag_upsert_text_auto_embed(idydb **handler,
                                     idydb_column_row_sizing text_column,
                                     idydb_column_row_sizing vector_column,
                                     idydb_column_row_sizing row,
                                     const char* text)
{
	if (!handler || !*handler || !(*handler)->embedder || !text) {
		idydb_error_state_if_available(handler, 8);
		return IDYDB_ERROR;
	}
	float* vec = NULL; unsigned short dims = 0;
	int erc = (*handler)->embedder(text, &vec, &dims, (*handler)->embedder_user);
	if (erc != 0 || !vec || dims == 0) {
		if (vec) free(vec);
		idydb_error_state_if_available(handler, 24);
		return IDYDB_ERROR;
	}
	int rc = idydb_rag_upsert_text(handler, text_column, vector_column, row, text, vec, dims);
	free(vec);
	return rc;
}

int idydb_rag_query_topk(idydb **handler,
                         idydb_column_row_sizing text_column,
                         idydb_column_row_sizing vector_column,
                         const float* query_embedding,
                         unsigned short dims,
                         unsigned short k,
                         idydb_similarity_metric metric,
                         idydb_knn_result* out_results,
                         char** out_texts)
{
	if (!handler || !*handler || !out_results || !out_texts) {
		idydb_error_state_if_available(handler, 8);
		return -1;
	}
	idydb_text_results_reset(out_texts, k);
	int n = idydb_knn_search_vector_column(handler, vector_column, query_embedding, dims, k, metric, out_results);
	if (n <= 0) return n;
	for (int i = 0; i < n; ++i) {
		if (out_results[i].row == 0) { out_texts[i] = NULL; continue; }
		int rc = idydb_extract(handler, text_column, out_results[i].row);
		if (rc != IDYDB_DONE) { out_texts[i] = NULL; continue; }
		if (idydb_retrieved_type(handler) != IDYDB_CHAR) { out_texts[i] = NULL; continue; }
		char* s = idydb_retrieve_char(handler);
		if (!s) { out_texts[i] = NULL; continue; }
		size_t len = strlen(s);
		out_texts[i] = (char*)malloc(len + 1);
		if (!out_texts[i]) {
			idydb_text_results_free(out_texts, k);
			idydb_error_state_if_available(handler, 24);
			return -1;
		}
		memcpy(out_texts[i], s, len + 1);
	}
	return n;
}

int idydb_rag_query_topk_filtered(idydb **handler,
                                  idydb_column_row_sizing text_column,
                                  idydb_column_row_sizing vector_column,
                                  const float* query_embedding,
                                  unsigned short dims,
                                  unsigned short k,
                                  idydb_similarity_metric metric,
                                  const idydb_filter* filter,
                                  idydb_knn_result* out_results,
                                  char** out_texts)
{
	if (!handler || !*handler || !out_results || !out_texts) {
		idydb_error_state_if_available(handler, 8);
		return -1;
	}
	idydb_text_results_reset(out_texts, k);

	int n = idydb_knn_search_vector_column_filtered(handler, vector_column, query_embedding, dims, k, metric, filter, out_results);
	if (n <= 0) return n;

	for (int i = 0; i < n; ++i) {
		if (out_results[i].row == 0) { out_texts[i] = NULL; continue; }
		int rc = idydb_extract(handler, text_column, out_results[i].row);
		if (rc != IDYDB_DONE) { out_texts[i] = NULL; continue; }
		if (idydb_retrieved_type(handler) != IDYDB_CHAR) { out_texts[i] = NULL; continue; }
		char* s = idydb_retrieve_char(handler);
		if (!s) { out_texts[i] = NULL; continue; }
		size_t len = strlen(s);
		out_texts[i] = (char*)malloc(len + 1);
		if (!out_texts[i]) {
			idydb_text_results_free(out_texts, k);
			idydb_error_state_if_available(handler, 24);
			return -1;
		}
		memcpy(out_texts[i], s, len + 1);
	}
	return n;
}

int idydb_rag_query_topk_with_metadata(idydb **handler,
                                       idydb_column_row_sizing text_column,
                                       idydb_column_row_sizing vector_column,
                                       const float* query_embedding,
                                       unsigned short dims,
                                       unsigned short k,
                                       idydb_similarity_metric metric,
                                       const idydb_filter* filter,
                                       const idydb_column_row_sizing* meta_columns,
                                       size_t meta_columns_count,
                                       idydb_knn_result* out_results,
                                       char** out_texts,
                                       idydb_value* out_meta)
{
	if (!handler || !*handler || !out_results || !out_texts) {
		idydb_error_state_if_available(handler, 8);
		return -1;
	}
	if (meta_columns_count > 0 && (!meta_columns || !out_meta)) {
		idydb_error_state_if_available(handler, 8);
		return -1;
	}

	for (unsigned short i = 0; i < k; ++i) out_texts[i] = NULL;
	if (out_meta && meta_columns_count > 0) {
		for (size_t i = 0; i < (size_t)k * meta_columns_count; ++i) out_meta[i].type = IDYDB_NULL;
	}

	int n = idydb_rag_query_topk_filtered(handler, text_column, vector_column, query_embedding, dims, k, metric,
	                                     filter, out_results, out_texts);
	if (n <= 0) return n;

	if (!out_meta || meta_columns_count == 0) return n;

	for (int i = 0; i < n; ++i)
	{
		if (out_results[i].row == 0) continue;
		for (size_t j = 0; j < meta_columns_count; ++j)
		{
			const size_t idx = (size_t)i * meta_columns_count + j;
			idydb_value* v = &out_meta[idx];
			v->type = IDYDB_NULL;

			int rc = idydb_extract(handler, meta_columns[j], out_results[i].row);
			if (rc == IDYDB_NULL) { v->type = IDYDB_NULL; continue; }
			if (rc != IDYDB_DONE) { v->type = IDYDB_NULL; continue; }

			unsigned char t = (unsigned char)idydb_retrieved_type(handler);
			v->type = t;

			switch (t)
			{
				case IDYDB_INTEGER:
					v->as.i = idydb_retrieve_int(handler);
					break;
				case IDYDB_FLOAT:
					v->as.f = idydb_retrieve_float(handler);
					break;
				case IDYDB_BOOL:
					v->as.b = idydb_retrieve_bool(handler);
					break;
				case IDYDB_CHAR:
				{
					char* s = idydb_retrieve_char(handler);
					if (!s) { v->type = IDYDB_NULL; break; }
					size_t len = strlen(s);
					v->as.s = (char*)malloc(len + 1);
					if (!v->as.s) { v->type = IDYDB_NULL; break; }
					memcpy(v->as.s, s, len + 1);
					break;
				}
				case IDYDB_VECTOR:
				{
					unsigned short vd = 0;
					const float* pv = idydb_retrieve_vector(handler, &vd);
					if (!pv || vd == 0) { v->type = IDYDB_NULL; break; }
					v->as.vec.dims = vd;
					v->as.vec.v = (float*)malloc(sizeof(float) * (size_t)vd);
					if (!v->as.vec.v) { v->type = IDYDB_NULL; v->as.vec.dims = 0; break; }
					memcpy(v->as.vec.v, pv, sizeof(float) * (size_t)vd);
					break;
				}
				default:
					v->type = IDYDB_NULL;
					break;
			}

			/* important: avoid keeping extracted VECTOR allocated on handler */
			idydb_clear_values(handler);
		}
	}

	return n;
}

int idydb_rag_query_context(idydb **handler,
                            idydb_column_row_sizing text_column,
                            idydb_column_row_sizing vector_column,
                            const float* query_embedding,
                            unsigned short dims,
                            unsigned short k,
                            idydb_similarity_metric metric,
                            size_t max_chars,
                            char** out_context)
{
	if (!handler || !*handler || !out_context) {
		idydb_error_state_if_available(handler, 8);
		return IDYDB_ERROR;
	}
	idydb_knn_result* res = (idydb_knn_result*)calloc(k, sizeof(idydb_knn_result));
	char** texts = (char**)calloc(k, sizeof(char*));
	if (!res || !texts) {
		if (res) free(res);
		if (texts) free(texts);
		idydb_error_state_if_available(handler, 24);
		return IDYDB_ERROR;
	}
	int n = idydb_rag_query_topk(handler, text_column, vector_column, query_embedding, dims, k, metric, res, texts);
	if (n <= 0) { free(res); free(texts); return (n == 0 ? IDYDB_DONE : IDYDB_ERROR); }
	size_t total = 0;
	const char* sep = "\n---\n";
	size_t sep_len = strlen(sep);
	for (int i = 0; i < n; ++i) { if (texts[i]) total += strlen(texts[i]); if (i+1 < n) total += sep_len; }
	if (max_chars > 0 && total > max_chars) total = max_chars;
	char* buf = (char*)malloc(total + 1);
	if (!buf) {
		for (int i = 0; i < n; ++i) if (texts[i]) free(texts[i]);
		free(res);
		free(texts);
		idydb_error_state_if_available(handler, 24);
		return IDYDB_ERROR;
	}
	size_t written = 0;
	for (int i = 0; i < n; ++i) {
		if (!texts[i]) continue;
		size_t len = strlen(texts[i]);
		if (max_chars > 0 && written + len > max_chars) len = max_chars - written;
		memcpy(buf + written, texts[i], len);
		written += len;
		if (i+1 < n) {
			if (max_chars > 0 && written + sep_len > max_chars) break;
			memcpy(buf + written, sep, sep_len);
			written += sep_len;
		}
	}
	buf[written] = '\0';
	for (int i=0;i<n;++i) if (texts[i]) free(texts[i]);
	free(res); free(texts);
	*out_context = buf;
	return IDYDB_DONE;
}

int idydb_rag_query_context_filtered(idydb **handler,
                                     idydb_column_row_sizing text_column,
                                     idydb_column_row_sizing vector_column,
                                     const float* query_embedding,
                                     unsigned short dims,
                                     unsigned short k,
                                     idydb_similarity_metric metric,
                                     const idydb_filter* filter,
                                     size_t max_chars,
                                     char** out_context)
{
	if (!handler || !*handler || !out_context) {
		idydb_error_state_if_available(handler, 8);
		return IDYDB_ERROR;
	}
	idydb_knn_result* res = (idydb_knn_result*)calloc(k, sizeof(idydb_knn_result));
	char** texts = (char**)calloc(k, sizeof(char*));
	if (!res || !texts) {
		if (res) free(res);
		if (texts) free(texts);
		idydb_error_state_if_available(handler, 24);
		return IDYDB_ERROR;
	}

	int n = idydb_rag_query_topk_filtered(handler, text_column, vector_column, query_embedding, dims, k, metric, filter, res, texts);
	if (n <= 0) { free(res); free(texts); return (n == 0 ? IDYDB_DONE : IDYDB_ERROR); }

	size_t total = 0;
	const char* sep = "\n---\n";
	size_t sep_len = strlen(sep);
	for (int i = 0; i < n; ++i) { if (texts[i]) total += strlen(texts[i]); if (i+1 < n) total += sep_len; }
	if (max_chars > 0 && total > max_chars) total = max_chars;
	char* buf = (char*)malloc(total + 1);
	if (!buf) {
		for (int i = 0; i < n; ++i) if (texts[i]) free(texts[i]);
		free(res);
		free(texts);
		idydb_error_state_if_available(handler, 24);
		return IDYDB_ERROR;
	}

	size_t written = 0;
	for (int i = 0; i < n; ++i) {
		if (!texts[i]) continue;
		size_t len = strlen(texts[i]);
		if (max_chars > 0 && written + len > max_chars) len = max_chars - written;
		memcpy(buf + written, texts[i], len);
		written += len;
		if (i+1 < n) {
			if (max_chars > 0 && written + sep_len > max_chars) break;
			memcpy(buf + written, sep, sep_len);
			written += sep_len;
		}
	}
	buf[written] = '\0';
	for (int i=0;i<n;++i) if (texts[i]) free(texts[i]);
	free(res); free(texts);
	*out_context = buf;
	return IDYDB_DONE;
}

/* ---------------- Query utilities (merged from db_query_utils.cpp) ---------------- */

