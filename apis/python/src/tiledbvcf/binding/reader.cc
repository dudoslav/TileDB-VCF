/**
 * @file   reader.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2019 TileDB, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <arrow/python/pyarrow.h>
#include <stdint.h>
#include <sys/types.h>
#include <tiledbvcf/arrow.h>
#include <cmath>
#include <stdexcept>

#include "arrow/type_fwd.h"
#include "reader.h"

namespace py = pybind11;

namespace {
void check_error(tiledb_vcf_reader_t* reader, int32_t rc) {
  if (rc != TILEDB_VCF_OK) {
    std::string msg =
        "TileDB-VCF-Py: Error getting tiledb_vcf_error_t error message.";
    tiledb_vcf_error_t* err = nullptr;
    const char* c_msg = nullptr;
    if (tiledb_vcf_reader_get_last_error(reader, &err) == TILEDB_VCF_OK &&
        tiledb_vcf_error_get_message(err, &c_msg) == TILEDB_VCF_OK) {
      msg = std::string(c_msg);
    }
    throw std::runtime_error(msg);
  }
}

void check_arrow_error(const arrow::Status& st) {
  if (!st.ok()) {
    std::string msg_str = "TileDB-VCF-Py Arrow error: " + st.message();
    throw std::runtime_error(msg_str);
  }
}
}  // namespace

namespace tiledbvcfpy {

void config_logging(const std::string& level, const std::string& logfile) {
  tiledb_vcf_config_logging(level.c_str(), logfile.c_str());
}

Reader::Reader()
    : ptr(nullptr, deleter)
    , mem_budget_mb_(2 * 1024) {
  tiledb_vcf_reader_t* r;
  if (tiledb_vcf_reader_alloc(&r) != TILEDB_VCF_OK)
    throw std::runtime_error(
        "TileDB-VCF-Py: Failed to allocate tiledb_vcf_reader_t instance.");
  ptr.reset(r);
}

void Reader::init(const std::string& dataset_uri) {
  auto reader = ptr.get();
  check_error(reader, tiledb_vcf_reader_init(reader, dataset_uri.c_str()));
}

void Reader::reset() {
  auto reader = ptr.get();
  check_error(reader, tiledb_vcf_reader_reset(reader));
}

void Reader::set_attributes(const std::vector<std::string>& attributes) {
  attributes_ = attributes;
}

void Reader::set_tiledb_stats_enabled(const bool stats_enabled) {
  auto reader = ptr.get();
  check_error(
      reader,
      tiledb_vcf_reader_set_tiledb_stats_enabled(reader, stats_enabled));
}

void Reader::set_samples(const std::string& samples) {
  auto reader = ptr.get();
  check_error(reader, tiledb_vcf_reader_set_samples(reader, samples.c_str()));
}

void Reader::set_samples_file(const std::string& uri) {
  auto reader = ptr.get();
  check_error(reader, tiledb_vcf_reader_set_samples_file(reader, uri.c_str()));
}

void Reader::set_regions(const std::string& regions) {
  auto reader = ptr.get();
  check_error(reader, tiledb_vcf_reader_set_regions(reader, regions.c_str()));
}

void Reader::set_bed_file(const std::string& uri) {
  auto reader = ptr.get();
  check_error(reader, tiledb_vcf_reader_set_bed_file(reader, uri.c_str()));
}

void Reader::set_bed_array(const std::string& uri) {
  auto reader = ptr.get();
  check_error(reader, tiledb_vcf_reader_set_bed_array(reader, uri.c_str()));
}

void Reader::set_region_partition(int32_t partition, int32_t num_partitions) {
  auto reader = ptr.get();
  check_error(
      reader,
      tiledb_vcf_reader_set_region_partition(
          reader, partition, num_partitions));
}

void Reader::set_sample_partition(int32_t partition, int32_t num_partitions) {
  auto reader = ptr.get();
  check_error(
      reader,
      tiledb_vcf_reader_set_sample_partition(
          reader, partition, num_partitions));
}

void Reader::set_memory_budget(int32_t memory_mb) {
  mem_budget_mb_ = memory_mb;

  // TileDB-VCF gets two thirds the budget, we use the other third for buffer
  // allocation.
  auto reader = ptr.get();
  check_error(
      reader,
      tiledb_vcf_reader_set_memory_budget(
          reader, uint64_t(mem_budget_mb_ / 3.0 * 2.0)));
}

void Reader::set_sort_regions(bool sort_regions) {
  auto reader = ptr.get();
  check_error(
      reader, tiledb_vcf_reader_set_sort_regions(reader, sort_regions ? 1 : 0));
}

void Reader::set_max_num_records(int64_t max_num_records) {
  auto reader = ptr.get();
  check_error(
      reader, tiledb_vcf_reader_set_max_num_records(reader, max_num_records));
}

void Reader::set_tiledb_config(const std::string& config_str) {
  auto reader = ptr.get();
  check_error(
      reader, tiledb_vcf_reader_set_tiledb_config(reader, config_str.c_str()));
}

void Reader::set_verbose(bool verbose) {
  auto reader = ptr.get();
  check_error(reader, tiledb_vcf_reader_set_verbose(reader, verbose));
}

void Reader::set_export_to_disk(bool export_to_disk) {
  auto reader = ptr.get();
  check_error(
      reader, tiledb_vcf_reader_set_export_to_disk(reader, export_to_disk));
}

void Reader::set_merge(bool merge) {
  auto reader = ptr.get();
  check_error(reader, tiledb_vcf_reader_set_merge(reader, merge));
}

void Reader::set_output_format(const std::string& output_format) {
  auto reader = ptr.get();
  check_error(
      reader,
      tiledb_vcf_reader_set_output_format(reader, output_format.c_str()));
}

void Reader::set_output_path(const std::string& output_path) {
  auto reader = ptr.get();
  check_error(
      reader, tiledb_vcf_reader_set_output_path(reader, output_path.c_str()));
}

void Reader::set_output_dir(const std::string& output_dir) {
  auto reader = ptr.get();
  check_error(
      reader, tiledb_vcf_reader_set_output_dir(reader, output_dir.c_str()));
}

void Reader::set_af_filter(const std::string& af_filter) {
  auto reader = ptr.get();
  check_error(
      reader, tiledb_vcf_reader_set_af_filter(reader, af_filter.c_str()));
}

void Reader::set_scan_all_samples(const bool scan_all_samples) {
  auto reader = ptr.get();
  check_error(
      reader, tiledb_vcf_reader_set_scan_all_samples(reader, scan_all_samples));
}

void Reader::read(const bool release_buffs) {
  auto reader = ptr.get();
  bool af_filter_enabled = false;
  if (tiledb_vcf_reader_get_af_filter_exists(reader, &af_filter_enabled) ==
      TILEDB_VCF_ERR)
    throw std::runtime_error("TileDB-VCF-Py: Error finding AF filter.");
  if (!af_filter_enabled) {
    for (std::string attribute : attributes_) {
      if (attribute == "info_TILEDB_IAF") {
        tiledb_vcf_reader_set_af_filter(reader, ">=0");
        af_filter_enabled = true;
      }
    }
  }
  if (af_filter_enabled) {
    // add alleles buffer only if not already present
    bool add_alleles = true;
    bool add_GT = true;
    for (std::string attribute : attributes_) {
      add_alleles = add_alleles && attribute != "alleles";
      add_GT = add_GT && attribute != "fmt_GT";
    }
    if (add_alleles) {
      attributes_.emplace_back("alleles");
    }
    if (add_GT) {
      attributes_.emplace_back("fmt_GT");
    }
  }
  alloc_buffers(release_buffs);
  set_buffers();

  {
    // Release python GIL while reading data from TileDB
    py::gil_scoped_release release;
    check_error(reader, tiledb_vcf_reader_read(reader));
  }

  tiledb_vcf_read_status_t status;
  check_error(reader, tiledb_vcf_reader_get_status(reader, &status));
  if (status != TILEDB_VCF_COMPLETED && status != TILEDB_VCF_INCOMPLETE)
    throw std::runtime_error(
        "TileDB-VCF-Py: Error submitting read; unhandled read status.");
}

void Reader::alloc_buffers(const bool release_buffs) {
  auto reader = ptr.get();

  // Release old buffers. TODO: reuse when possible
  if (release_buffs)
    release_buffers();

  // Get a count of the number of buffers required.
  int num_buffers = 0;
  for (const auto& attr : attributes_) {
    tiledb_vcf_attr_datatype_t datatype = TILEDB_VCF_UINT8;
    int32_t var_len = 0, nullable = 0, list = 0;
    check_error(
        reader,
        tiledb_vcf_reader_get_attribute_type(
            reader, attr.c_str(), &datatype, &var_len, &nullable, &list));
    num_buffers += 1;
    num_buffers += var_len ? 1 : 0;
    num_buffers += nullable ? 1 : 0;
    num_buffers += list ? 1 : 0;
  }

  if (num_buffers == 0)
    return;

  // Only use one third the budget because TileDB-VCF gets the other two thirds.
  const int64_t budget_mb = mem_budget_mb_ / 3.0;

  // The undocumented "0 MB" budget is used only for testing incomplete queries.
  int64_t alloc_size_bytes;
  if (mem_budget_mb_ == 0) {
    alloc_size_bytes = 10;  // Some small value
  } else {
    alloc_size_bytes = (budget_mb * 1024 * 1024) / num_buffers;
  }

  for (const auto& attr : attributes_) {
    tiledb_vcf_attr_datatype_t datatype = TILEDB_VCF_UINT8;
    int32_t var_len = 0, nullable = 0, list = 0;
    check_error(
        reader,
        tiledb_vcf_reader_get_attribute_type(
            reader, attr.c_str(), &datatype, &var_len, &nullable, &list));

    buffers_.emplace_back();
    BufferInfo& buffer = buffers_.back();
    buffer.attr_name = attr;

    auto dtype = to_numpy_dtype(datatype);
    buffer.datatype = datatype;
    buffer.arrow_datatype = to_arrow_datatype(datatype);
    buffer.arrow_array_datatype = to_arrow_datatype(datatype);
    size_t count = alloc_size_bytes / dtype.itemsize();

    auto maybe_buffer = arrow::AllocateBuffer(alloc_size_bytes);
    if (!maybe_buffer.ok()) {
      throw std::runtime_error(
          "TileDB-VCF-Py: nullable bitmap buffer allocation failed");
    } else {
      buffer.data = std::move(*maybe_buffer);
    }

    if (var_len == 1) {
      auto maybe_buffer = arrow::AllocateBuffer(alloc_size_bytes);
      if (!maybe_buffer.ok()) {
        throw std::runtime_error(
            "TileDB-VCF-Py: offset buffer allocation failed");
      } else {
        buffer.offsets = std::move(*maybe_buffer);
      }

      // Make list type (can be list of list if also list below), exclude double
      // counting strings
      if (datatype != TILEDB_VCF_CHAR)
        buffer.arrow_array_datatype = arrow::list(buffer.arrow_array_datatype);
    }

    if (list == 1) {
      auto maybe_buffer = arrow::AllocateBuffer(alloc_size_bytes);
      if (!maybe_buffer.ok()) {
        throw std::runtime_error(
            "TileDB-VCF-Py: list offset buffer allocation failed");
      } else {
        buffer.list_offsets = std::move(*maybe_buffer);
      }

      // Make list type (can be list of list if also var_length)
      buffer.arrow_array_datatype = arrow::list(buffer.arrow_array_datatype);
    }

    if (nullable == 1) {
      auto maybe_buffer = arrow::AllocateBuffer(alloc_size_bytes);
      if (!maybe_buffer.ok()) {
        throw std::runtime_error(
            "TileDB-VCF-Py: nullable bitmap buffer allocation failed");
      } else {
        buffer.bitmap = std::move(*maybe_buffer);
      }
    }

    buffer.array = build_arrow_array_from_buffer(buffer, count, count, count);
  }
}

std::shared_ptr<arrow::Array> Reader::build_arrow_array_from_buffer(
    BufferInfo& buffer,
    const uint64_t& count,
    const uint64_t& num_offsets,
    const uint64_t& num_data_elements) {
  std::shared_ptr<arrow::Array> array;
  if (buffer.datatype == TILEDB_VCF_CHAR) {
    if (buffer.list_offsets != nullptr) {
      auto data_array = std::make_shared<arrow::StringArray>(
          num_offsets - 1, buffer.offsets, buffer.data);
      array = std::make_shared<arrow::ListArray>(
          arrow::list(buffer.arrow_datatype),
          count,
          buffer.list_offsets,
          data_array,
          buffer.bitmap);
    } else {
      array = std::make_shared<arrow::StringArray>(
          count, buffer.offsets, buffer.data, buffer.bitmap);
    }
  } else if (buffer.datatype == TILEDB_VCF_UINT8) {
    array = build_arrow_array<arrow::UInt8Array>(
        buffer, count, num_offsets, num_data_elements);
  } else if (buffer.datatype == TILEDB_VCF_INT32) {
    array = build_arrow_array<arrow::Int32Array>(
        buffer, count, num_offsets, num_data_elements);
  } else if (buffer.datatype == TILEDB_VCF_FLOAT32) {
    array = build_arrow_array<arrow::FloatArray>(
        buffer, count, num_offsets, num_data_elements);
  } else {
    throw std::runtime_error(
        "TileDB-VCF-Py: unknown datatype for arrow creation: " +
        std::to_string(buffer.datatype));
  }
  return array;
}

void Reader::set_buffers() {
  auto reader = ptr.get();
  for (auto& buff : buffers_) {
    const auto& attr = buff.attr_name;
    auto offsets = buff.offsets;
    auto list_offsets = buff.list_offsets;
    auto data = buff.data;
    auto bitmap = buff.bitmap;

    check_error(
        reader,
        tiledb_vcf_reader_set_buffer_values(
            reader, attr.c_str(), data->size(), data->mutable_data()));

    if (offsets != nullptr)
      check_error(
          reader,
          tiledb_vcf_reader_set_buffer_offsets(
              reader,
              attr.c_str(),
              offsets->size(),
              reinterpret_cast<int32_t*>(offsets->mutable_data())));

    if (list_offsets != nullptr)
      check_error(
          reader,
          tiledb_vcf_reader_set_buffer_list_offsets(
              reader,
              attr.c_str(),
              list_offsets->size(),
              reinterpret_cast<int32_t*>(list_offsets->mutable_data())));

    if (bitmap != nullptr)
      check_error(
          reader,
          tiledb_vcf_reader_set_buffer_validity_bitmap(
              reader, attr.c_str(), bitmap->size(), bitmap->mutable_data()));
  }
}

void Reader::release_buffers() {
  auto reader = ptr.get();
  check_error(reader, tiledb_vcf_reader_reset_buffers(reader));

  buffers_.clear();
}

py::object Reader::get_results_arrow() {
  auto reader = ptr.get();

  std::vector<std::shared_ptr<arrow::Field>> fields;
  std::vector<std::shared_ptr<arrow::Array>> arrays;

  int64_t num_records = result_num_records();
  for (auto& buffer : buffers_) {
    std::shared_ptr<arrow::Field> field =
        arrow::field(buffer.attr_name, buffer.arrow_array_datatype);
    fields.push_back(field);

    // build new arrow::array based on the result size
    int64_t num_offsets = 0, num_data_elements = 0, num_data_bytes = 0;
    check_error(
        reader,
        tiledb_vcf_reader_get_result_size(
            reader,
            buffer.attr_name.c_str(),
            &num_offsets,
            &num_data_elements,
            &num_data_bytes));

    std::shared_ptr<arrow::Array> array = build_arrow_array_from_buffer(
        buffer, num_records, num_offsets, num_data_elements);
    arrays.push_back(array);
  }

  std::shared_ptr<arrow::Schema> schema = arrow::schema(fields);
  auto table = arrow::Table::Make(schema, arrays, num_records);
  check_arrow_error(table->Validate());

  PyObject* obj = arrow::py::wrap_table(table);
  if (obj == nullptr) {
    PyErr_PrintEx(1);
    throw std::runtime_error(
        "TileDB-VCF-Py: Error converting to Arrow; null Python object.");
  }
  return py::reinterpret_steal<py::object>(obj);
}

int64_t Reader::result_num_records() {
  auto reader = ptr.get();
  int64_t result = 0;
  check_error(
      reader, tiledb_vcf_reader_get_result_num_records(reader, &result));
  return result;
}

bool Reader::completed() {
  auto reader = ptr.get();
  tiledb_vcf_read_status_t status;
  check_error(reader, tiledb_vcf_reader_get_status(reader, &status));
  return status == TILEDB_VCF_COMPLETED;
}

bool Reader::get_tiledb_stats_enabled() {
  auto reader = ptr.get();
  bool stats_enabled;
  check_error(
      reader,
      tiledb_vcf_reader_get_tiledb_stats_enabled(reader, &stats_enabled));
  return stats_enabled;
}

std::string Reader::get_tiledb_stats() {
  auto reader = ptr.get();
  char* stats;
  check_error(reader, tiledb_vcf_reader_get_tiledb_stats(reader, &stats));
  return std::string(stats);
}

int32_t Reader::get_schema_version() {
  auto reader = ptr.get();
  int32_t version;
  check_error(reader, tiledb_vcf_reader_get_dataset_version(reader, &version));
  return version;
}

std::vector<std::string> Reader::get_fmt_attributes() {
  auto reader = ptr.get();
  int32_t count;
  std::vector<std::string> attrs;
  check_error(
      reader, tiledb_vcf_reader_get_fmt_attribute_count(reader, &count));

  for (int32_t i = 0; i < count; i++) {
    char* name;
    check_error(
        reader, tiledb_vcf_reader_get_fmt_attribute_name(reader, i, &name));
    attrs.emplace_back(name);
  }

  return attrs;
}

std::vector<std::string> Reader::get_info_attributes() {
  auto reader = ptr.get();
  int32_t count;
  std::vector<std::string> attrs;
  check_error(
      reader, tiledb_vcf_reader_get_info_attribute_count(reader, &count));

  for (int32_t i = 0; i < count; i++) {
    char* name;
    check_error(
        reader, tiledb_vcf_reader_get_info_attribute_name(reader, i, &name));
    attrs.emplace_back(name);
  }

  return attrs;
}

std::vector<std::string> Reader::get_queryable_attributes() {
  auto reader = ptr.get();
  int32_t count;
  std::vector<std::string> attrs;
  check_error(
      reader, tiledb_vcf_reader_get_queryable_attribute_count(reader, &count));

  for (int32_t i = 0; i < count; i++) {
    char* name;
    check_error(
        reader,
        tiledb_vcf_reader_get_queryable_attribute_name(reader, i, &name));
    attrs.emplace_back(name);
  }

  return attrs;
}

std::vector<std::string> Reader::get_materialized_attributes() {
  auto reader = ptr.get();
  int32_t count;
  std::vector<std::string> attrs;
  check_error(
      reader,
      tiledb_vcf_reader_get_materialized_attribute_count(reader, &count));

  for (int32_t i = 0; i < count; i++) {
    char* name;
    check_error(
        reader,
        tiledb_vcf_reader_get_materialized_attribute_name(reader, i, &name));
    attrs.emplace_back(name);
  }

  return attrs;
}

int32_t Reader::get_sample_count() {
  auto reader = ptr.get();
  int32_t count;
  check_error(reader, tiledb_vcf_reader_get_sample_count(reader, &count));
  return count;
}

std::vector<std::string> Reader::get_sample_names() {
  auto reader = ptr.get();
  int32_t count;
  std::vector<std::string> names;
  check_error(reader, tiledb_vcf_reader_get_sample_count(reader, &count));

  names.reserve(count);
  for (int32_t i = 0; i < count; i++) {
    const char* name;
    check_error(reader, tiledb_vcf_reader_get_sample_name(reader, i, &name));
    names.emplace_back(name);
  }

  return names;
}

py::object Reader::get_variant_stats_results() {
  std::shared_ptr<arrow::Field> pos_field =
      arrow::field("pos", arrow::uint32());
  std::shared_ptr<arrow::Field> allele_field =
      arrow::field("allele", arrow::large_utf8());
  std::shared_ptr<arrow::Field> ac_field = arrow::field("ac", arrow::int32());
  std::shared_ptr<arrow::Field> an_field = arrow::field("an", arrow::int32());
  std::shared_ptr<arrow::Field> af_field = arrow::field("af", arrow::float32());
  std::vector<std::shared_ptr<arrow::Field>> fields = {
      pos_field, allele_field, ac_field, an_field, af_field};

  size_t cardinality, alleles_size;
  auto reader = ptr.get();

  check_error(reader, tiledb_vcf_reader_prepare_variant_stats(reader));
  check_error(
      reader,
      tiledb_vcf_reader_get_variant_stats_buffer_sizes(
          reader, &cardinality, &alleles_size));

  auto maybe_pos_buffer = arrow::AllocateBuffer(cardinality * sizeof(uint32_t));
  auto maybe_allele_buffer = arrow::AllocateBuffer(alleles_size);
  auto maybe_allele_offset_buffer =
      arrow::AllocateBuffer((cardinality + 1) * sizeof(uint64_t));
  auto maybe_ac_buffer = arrow::AllocateBuffer(cardinality * sizeof(int32_t));
  auto maybe_an_buffer = arrow::AllocateBuffer(cardinality * sizeof(int32_t));
  auto maybe_af_buffer = arrow::AllocateBuffer(cardinality * sizeof(float_t));
  if (!(maybe_pos_buffer.ok() && maybe_allele_buffer.ok() &&
        maybe_allele_offset_buffer.ok() && maybe_ac_buffer.ok() &&
        maybe_an_buffer.ok() && maybe_af_buffer.ok())) {
    throw std::runtime_error(
        "TileDB-VCF-Py: buffer allocation failed for fetching stats");
  }
  std::shared_ptr<arrow::Buffer> pos_buffer = std::move(*maybe_pos_buffer);
  std::shared_ptr<arrow::Buffer> allele_buffer =
      std::move(*maybe_allele_buffer);
  std::shared_ptr<arrow::Buffer> allele_offset_buffer =
      std::move(*maybe_allele_offset_buffer);
  std::shared_ptr<arrow::Buffer> ac_buffer = std::move(*maybe_ac_buffer);
  std::shared_ptr<arrow::Buffer> an_buffer = std::move(*maybe_an_buffer);
  std::shared_ptr<arrow::Buffer> af_buffer = std::move(*maybe_af_buffer);

  check_error(
      reader,
      tiledb_vcf_reader_read_from_variant_stats(
          reader,
          reinterpret_cast<uint32_t*>(pos_buffer->mutable_data()),
          reinterpret_cast<char*>(allele_buffer->mutable_data()),
          reinterpret_cast<uint64_t*>(allele_offset_buffer->mutable_data()),
          reinterpret_cast<int*>(ac_buffer->mutable_data()),
          reinterpret_cast<int*>(an_buffer->mutable_data()),
          reinterpret_cast<float_t*>(af_buffer->mutable_data())));

  std::shared_ptr<arrow::Array> pos_array =
      std::make_shared<arrow::UInt32Array>(cardinality, pos_buffer);
  std::shared_ptr<arrow::Array> allele_array =
      std::make_shared<arrow::LargeStringArray>(
          cardinality, allele_offset_buffer, allele_buffer);
  std::shared_ptr<arrow::Array> ac_array =
      std::make_shared<arrow::Int32Array>(cardinality, ac_buffer);
  std::shared_ptr<arrow::Array> an_array =
      std::make_shared<arrow::Int32Array>(cardinality, an_buffer);
  std::shared_ptr<arrow::Array> af_array =
      std::make_shared<arrow::FloatArray>(cardinality, af_buffer);
  std::vector<std::shared_ptr<arrow::Array>> arrays = {
      pos_array, allele_array, ac_array, an_array, af_array};

  std::shared_ptr<arrow::Schema> schema = arrow::schema(fields);
  auto table = arrow::Table::Make(schema, arrays, cardinality);
  check_arrow_error(table->Validate());

  PyObject* obj = arrow::py::wrap_table(table);
  if (obj == nullptr) {
    PyErr_PrintEx(1);
    throw std::runtime_error(
        "TileDB-VCF-Py: Error converting stats export table to Arrow; null "
        "Python object.");
  }

  return py::reinterpret_steal<py::object>(obj);
}

py::dtype Reader::to_numpy_dtype(tiledb_vcf_attr_datatype_t datatype) {
  switch (datatype) {
    case TILEDB_VCF_CHAR:
      return py::dtype("S1");
    case TILEDB_VCF_UINT8:
      return py::dtype::of<uint8_t>();
    case TILEDB_VCF_INT32:
      return py::dtype::of<int32_t>();
    case TILEDB_VCF_FLOAT32:
      return py::dtype::of<float>();
    default:
      throw std::runtime_error(
          "TileDB-VCF-Py: Error converting to numpy dtype; unhandled "
          "datatype " +
          std::to_string(datatype));
  }
}

std::shared_ptr<arrow::DataType> Reader::to_arrow_datatype(
    tiledb_vcf_attr_datatype_t datatype) {
  switch (datatype) {
    case TILEDB_VCF_CHAR:
      return arrow::utf8();
    case TILEDB_VCF_UINT8:
      return arrow::uint8();
    case TILEDB_VCF_INT32:
      return arrow::int32();
    case TILEDB_VCF_FLOAT32:
      return arrow::float32();
    default:
      throw std::runtime_error(
          "TileDB-VCF-Py: Error converting to arrow datatype; unhandled "
          "datatype " +
          std::to_string(datatype));
  }
}

void Reader::deleter(tiledb_vcf_reader_t* r) {
  tiledb_vcf_reader_free(&r);
}

void Reader::set_buffer_percentage(float buffer_percentage) {
  auto reader = ptr.get();
  check_error(
      reader,
      tiledb_vcf_reader_set_buffer_percentage(reader, buffer_percentage));
}

void Reader::set_tiledb_tile_cache_percentage(float tile_percentage) {
  auto reader = ptr.get();
  check_error(
      reader,
      tiledb_vcf_reader_set_tiledb_tile_cache_percentage(
          reader, tile_percentage));
}

void Reader::set_check_samples_exist(bool samples_exists) {
  auto reader = ptr.get();
  check_error(
      reader,
      tiledb_vcf_reader_set_check_samples_exist(reader, samples_exists));
}

std::string Reader::version() {
  const char* version_str;
  tiledb_vcf_version(&version_str);
  return version_str;
}

void Reader::set_enable_progress_estimation(
    const bool& enable_progress_estimation) {
  auto reader = ptr.get();
  check_error(
      reader,
      tiledb_vcf_reader_set_enable_progress_estimation(
          reader, enable_progress_estimation));
}

void Reader::set_debug_print_vcf_regions(const bool& print_vcf_regions) {
  auto reader = ptr.get();
  check_error(
      reader,
      tiledb_vcf_reader_set_debug_print_vcf_regions(reader, print_vcf_regions));
}

void Reader::set_debug_print_sample_list(const bool& print_sample_list) {
  auto reader = ptr.get();
  check_error(
      reader,
      tiledb_vcf_reader_set_debug_print_sample_list(reader, print_sample_list));
}

void Reader::set_debug_print_tiledb_query_ranges(
    const bool& tiledb_query_ranges) {
  auto reader = ptr.get();
  check_error(
      reader,
      tiledb_vcf_reader_set_debug_print_tiledb_query_ranges(
          reader, tiledb_query_ranges));
}
}  // namespace tiledbvcfpy
