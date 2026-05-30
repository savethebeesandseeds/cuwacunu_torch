namespace db {
namespace query {
namespace {

[[nodiscard]] bool is_valid_handler(idydb **handler)
{
	return handler != nullptr && *handler != nullptr;
}

void set_error(std::string *error, std::string_view msg)
{
	if (error) *error = std::string(msg);
}

[[nodiscard]] bool copy_current_value(idydb **handler, idydb_value *out, std::string *error)
{
	if (!out)
	{
		set_error(error, "output value pointer is null");
		return false;
	}
	out->type = IDYDB_NULL;
	if (!is_valid_handler(handler))
	{
		set_error(error, "handler is null");
		return false;
	}

	const int t = idydb_retrieved_type(handler);
	out->type = static_cast<unsigned char>(t);
	switch (t)
	{
		case IDYDB_NULL:
			return true;
		case IDYDB_INTEGER:
			out->as.i = idydb_retrieve_int(handler);
			return true;
		case IDYDB_FLOAT:
			out->as.f = idydb_retrieve_float(handler);
			return true;
		case IDYDB_BOOL:
			out->as.b = idydb_retrieve_bool(handler);
			return true;
		case IDYDB_CHAR:
		{
			const char *s = idydb_retrieve_char(handler);
			if (!s)
			{
				out->type = IDYDB_NULL;
				return true;
			}
			const std::size_t n = ::strlen(s);
			out->as.s = static_cast<char *>(std::malloc(n + 1));
			if (!out->as.s)
			{
				set_error(error, "allocation failed while copying char value");
				out->type = IDYDB_NULL;
				return false;
			}
			::memcpy(out->as.s, s, n + 1);
			return true;
		}
		case IDYDB_VECTOR:
		{
			unsigned short dims = 0;
			const float *v = idydb_retrieve_vector(handler, &dims);
			if (!v || dims == 0)
			{
				out->type = IDYDB_NULL;
				out->as.vec.v = nullptr;
				out->as.vec.dims = 0;
				return true;
			}
			out->as.vec.v = static_cast<float *>(
			    std::malloc(sizeof(float) * static_cast<std::size_t>(dims)));
			if (!out->as.vec.v)
			{
				set_error(error, "allocation failed while copying vector value");
				out->type = IDYDB_NULL;
				out->as.vec.dims = 0;
				return false;
			}
			out->as.vec.dims = dims;
			::memcpy(out->as.vec.v, v, sizeof(float) * static_cast<std::size_t>(dims));
			return true;
		}
		default:
			out->type = IDYDB_NULL;
			return true;
	}
}

[[nodiscard]] bool filter_cmp_int(int a, idydb_filter_op op, int b)
{
	switch (op)
	{
		case IDYDB_FILTER_OP_EQ:
			return a == b;
		case IDYDB_FILTER_OP_NEQ:
			return a != b;
		case IDYDB_FILTER_OP_GT:
			return a > b;
		case IDYDB_FILTER_OP_GTE:
			return a >= b;
		case IDYDB_FILTER_OP_LT:
			return a < b;
		case IDYDB_FILTER_OP_LTE:
			return a <= b;
		default:
			return false;
	}
}

[[nodiscard]] bool filter_cmp_float(float a, idydb_filter_op op, float b)
{
	switch (op)
	{
		case IDYDB_FILTER_OP_EQ:
			return a == b;
		case IDYDB_FILTER_OP_NEQ:
			return a != b;
		case IDYDB_FILTER_OP_GT:
			return a > b;
		case IDYDB_FILTER_OP_GTE:
			return a >= b;
		case IDYDB_FILTER_OP_LT:
			return a < b;
		case IDYDB_FILTER_OP_LTE:
			return a <= b;
		default:
			return false;
	}
}

[[nodiscard]] bool filter_cmp_bool(bool a, idydb_filter_op op, bool b)
{
	switch (op)
	{
		case IDYDB_FILTER_OP_EQ:
			return a == b;
		case IDYDB_FILTER_OP_NEQ:
			return a != b;
		default:
			return false;
	}
}

[[nodiscard]] bool filter_matches_cell(idydb **handler, const idydb_filter_term &term,
                                       idydb_column_row_sizing row, std::string *error)
{
	if (!is_valid_handler(handler))
	{
		set_error(error, "handler is null");
		return false;
	}

	int rc = idydb_extract(handler, term.column, row);
	if (rc == IDYDB_NULL)
	{
		return term.op == IDYDB_FILTER_OP_IS_NULL ||
		       (term.op == IDYDB_FILTER_OP_EQ && term.type == IDYDB_NULL);
	}
	if (rc != IDYDB_DONE)
	{
		std::ostringstream oss;
		oss << "extract failed for filter column=" << term.column << " row=" << row
		    << " rc=" << rc;
		set_error(error, oss.str());
		return false;
	}

	if (term.op == IDYDB_FILTER_OP_IS_NOT_NULL ||
	    (term.op == IDYDB_FILTER_OP_NEQ && term.type == IDYDB_NULL))
	{
		return true;
	}
	if (term.op == IDYDB_FILTER_OP_IS_NULL ||
	    (term.op == IDYDB_FILTER_OP_EQ && term.type == IDYDB_NULL))
	{
		return false;
	}

	const int actual_type = idydb_retrieved_type(handler);
	if (actual_type == IDYDB_NULL) return false;

	if (term.type == IDYDB_INTEGER && actual_type == IDYDB_INTEGER)
	{
		return filter_cmp_int(idydb_retrieve_int(handler), term.op, term.value.i);
	}
	if (term.type == IDYDB_FLOAT && actual_type == IDYDB_FLOAT)
	{
		return filter_cmp_float(idydb_retrieve_float(handler), term.op, term.value.f);
	}
	if (term.type == IDYDB_BOOL && actual_type == IDYDB_BOOL)
	{
		return filter_cmp_bool(idydb_retrieve_bool(handler), term.op, term.value.b);
	}
	if (term.type == IDYDB_CHAR && actual_type == IDYDB_CHAR)
	{
		const char *v = idydb_retrieve_char(handler);
		const char *w = term.value.s ? term.value.s : "";
		const bool eq = v != nullptr && ::strcmp(v, w) == 0;
		if (term.op == IDYDB_FILTER_OP_EQ) return eq;
		if (term.op == IDYDB_FILTER_OP_NEQ) return !eq;
		return false;
	}

	return false;
}

[[nodiscard]] std::vector<idydb_column_row_sizing> referenced_columns(const query_spec_t &spec)
{
	std::set<idydb_column_row_sizing> uniq;
	for (const auto &c : spec.select_columns)
	{
		if (c > 0) uniq.insert(c);
	}
	for (const auto &f : spec.filters)
	{
		if (f.column > 0) uniq.insert(f.column);
	}
	return std::vector<idydb_column_row_sizing>(uniq.begin(), uniq.end());
}

[[nodiscard]] idydb_column_row_sizing max_candidate_row(
    idydb **handler, const std::vector<idydb_column_row_sizing> &cols)
{
	idydb_column_row_sizing max_row = 1;
	for (const auto c : cols)
	{
		const auto next = idydb_column_next_row(handler, c);
		if (next > max_row) max_row = next;
	}
	return max_row;
}

} // namespace

bool select_rows(idydb **handler, const query_spec_t &spec,
                 std::vector<idydb_column_row_sizing> *out_rows, std::string *error)
{
	if (error) error->clear();
	if (!out_rows)
	{
		set_error(error, "out_rows is null");
		return false;
	}
	out_rows->clear();
	if (!is_valid_handler(handler))
	{
		set_error(error, "handler is null");
		return false;
	}

	const auto cols = referenced_columns(spec);
	if (cols.empty()) return true;

	const idydb_column_row_sizing max_row = max_candidate_row(handler, cols);
	if (max_row <= 1) return true;

	std::vector<idydb_column_row_sizing> matches{};
	matches.reserve(static_cast<std::size_t>(max_row));

	for (idydb_column_row_sizing row = 1; row < max_row; ++row)
	{
		bool ok = true;
		for (const auto &filter : spec.filters)
		{
			if (!filter_matches_cell(handler, filter, row, error))
			{
				if (error && !error->empty()) return false;
				ok = false;
				break;
			}
		}
		if (ok) matches.push_back(row);
	}

	if (spec.newest_first) std::reverse(matches.begin(), matches.end());

	const std::size_t off = std::min(spec.offset, matches.size());
	std::size_t lim = spec.limit;
	if (lim == 0) lim = matches.size() - off;
	lim = std::min(lim, matches.size() - off);

	out_rows->assign(matches.begin() + static_cast<std::ptrdiff_t>(off),
	                 matches.begin() + static_cast<std::ptrdiff_t>(off + lim));
	return true;
}

bool fetch_row(idydb **handler, idydb_column_row_sizing row,
               const std::vector<idydb_column_row_sizing> &columns,
               std::vector<idydb_value> *out_values, std::string *error)
{
	if (error) error->clear();
	if (!out_values)
	{
		set_error(error, "out_values is null");
		return false;
	}
	out_values->clear();
	if (!is_valid_handler(handler))
	{
		set_error(error, "handler is null");
		return false;
	}
	if (row == 0)
	{
		set_error(error, "row must be >= 1");
		return false;
	}

	out_values->reserve(columns.size());
	for (const auto c : columns)
	{
		idydb_value v{};
		v.type = IDYDB_NULL;
		if (c == 0)
		{
			out_values->push_back(v);
			continue;
		}
		const int rc = idydb_extract(handler, c, row);
		if (rc == IDYDB_NULL)
		{
			out_values->push_back(v);
			continue;
		}
		if (rc != IDYDB_DONE)
		{
			std::ostringstream oss;
			oss << "extract failed for column=" << c << " row=" << row << " rc=" << rc;
			set_error(error, oss.str());
			idydb_values_free(out_values->data(), out_values->size());
			out_values->clear();
			return false;
		}
		if (!copy_current_value(handler, &v, error))
		{
			idydb_values_free(out_values->data(), out_values->size());
			out_values->clear();
			return false;
		}
		out_values->push_back(v);
	}
	return true;
}

bool fetch_rows(idydb **handler, const std::vector<idydb_column_row_sizing> &rows,
                const std::vector<idydb_column_row_sizing> &columns,
                std::vector<query_row_t> *out_rows, std::string *error)
{
	if (error) error->clear();
	if (!out_rows)
	{
		set_error(error, "out_rows is null");
		return false;
	}
	out_rows->clear();
	if (!is_valid_handler(handler))
	{
		set_error(error, "handler is null");
		return false;
	}

	out_rows->reserve(rows.size());
	for (const auto row : rows)
	{
		query_row_t q{};
		q.row_id = row;
		if (!fetch_row(handler, row, columns, &q.values, error))
		{
			for (auto &it : *out_rows)
			{
				if (!it.values.empty()) idydb_values_free(it.values.data(), it.values.size());
				it.values.clear();
			}
			out_rows->clear();
			return false;
		}
		out_rows->push_back(std::move(q));
	}
	return true;
}

bool count_rows(idydb **handler, const std::vector<idydb_filter_term> &filters,
                std::size_t *out_count, std::string *error)
{
	if (error) error->clear();
	if (!out_count)
	{
		set_error(error, "out_count is null");
		return false;
	}
	*out_count = 0;

	query_spec_t spec{};
	spec.filters = filters;
	for (const auto &f : filters)
	{
		if (f.column > 0) spec.select_columns.push_back(f.column);
	}
	std::vector<idydb_column_row_sizing> rows{};
	if (!select_rows(handler, spec, &rows, error)) return false;
	*out_count = rows.size();
	return true;
}

bool latest_row_by_key(idydb **handler, idydb_column_row_sizing key_column,
                       const std::string &key_value, idydb_column_row_sizing *out_row,
                       std::string *error)
{
	if (error) error->clear();
	if (!out_row)
	{
		set_error(error, "out_row is null");
		return false;
	}
	*out_row = 0;
	if (!is_valid_handler(handler))
	{
		set_error(error, "handler is null");
		return false;
	}
	if (key_column == 0)
	{
		set_error(error, "key_column must be >= 1");
		return false;
	}

	idydb_filter_term f{};
	f.column = key_column;
	f.type = IDYDB_CHAR;
	f.op = IDYDB_FILTER_OP_EQ;
	f.value.s = key_value.c_str();

	query_spec_t spec{};
	spec.filters.push_back(f);
	spec.select_columns.push_back(key_column);
	spec.limit = 1;
	spec.newest_first = true;

	std::vector<idydb_column_row_sizing> rows{};
	if (!select_rows(handler, spec, &rows, error)) return false;
	if (!rows.empty()) *out_row = rows.front();
	return true;
}

bool exists_row(idydb **handler, const std::vector<idydb_filter_term> &filters,
                bool *out_exists, std::string *error)
{
	if (error) error->clear();
	if (!out_exists)
	{
		set_error(error, "out_exists is null");
		return false;
	}
	*out_exists = false;
	if (!is_valid_handler(handler))
	{
		set_error(error, "handler is null");
		return false;
	}

	query_spec_t spec{};
	spec.filters = filters;
	for (const auto &f : filters)
	{
		if (f.column > 0) spec.select_columns.push_back(f.column);
	}
	spec.limit = 1;

	std::vector<idydb_column_row_sizing> rows{};
	if (!select_rows(handler, spec, &rows, error)) return false;
	*out_exists = !rows.empty();
	return true;
}

} // namespace query
} // namespace db

/* ---------------- undefines mirroring original ---------------- */

#undef IDYDB_MAX_BUFFER_SIZE
#undef IDYDB_MAX_CHAR_LENGTH
#undef IDYDB_MAX_VECTOR_DIM
#undef IDYDB_MAX_ERR_SIZE
#undef IDYDB_COLUMN_POSITION_MAX
#undef IDYDB_ROW_POSITION_MAX
#undef IDYDB_SEGMENT_SIZE
#undef IDYDB_PARTITION_SIZE
#undef IDYDB_PARTITION_AND_SEGMENT
#undef IDYDB_READ_INT
#undef IDYDB_READ_FLOAT
#undef IDYDB_READ_CHAR
#undef IDYDB_READ_BOOL_TRUE
#undef IDYDB_READ_BOOL_FALSE
#undef IDYDB_READ_VECTOR
#undef IDYDB_MMAP_OK
#undef IDYDB_ALLOW_UNSAFE
#undef IDYDB_MMAP_ALLOWED

