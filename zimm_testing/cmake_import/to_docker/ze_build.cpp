#include <zimm/target.hpp>
#include <zimm/third_party_target.hpp>

#include <filesystem>
#include <format>
#include <iostream>
#include <map>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

using namespace zimm;

namespace fs = std::filesystem;

namespace
{

struct Expected
{
    std::string_view name;
    TargetType type;
};

// A dependency whose manifest an earlier row found and a later row's config references
// (e.g. absl links GTest::gtest): fabricated into the later row's wrapper under `ns`.
struct DepSpec
{
    std::string_view pkg; // key of the already-found manifest
    std::string_view ns;  // cmake namespace its targets are exposed under
};

struct Package
{
    std::string_view cmakeName;
    std::string_view cmakeArgs;
    std::vector<Expected> expected;
    std::vector<DepSpec> deps = {};
    std::string_view hints = {};
};

using TargetType::Executable, TargetType::StaticLibrary, TargetType::SharedLibrary,
    TargetType::HeaderOnlyLibrary;

// clang-format off
const std::vector<Package> packages = {
    {"fmt", "",
     { {"fmt", SharedLibrary}, {"fmt-header-only", HeaderOnlyLibrary}}},
    {"spdlog", "",
     { {"Threads", HeaderOnlyLibrary}, {"fmt", SharedLibrary}, {"fmt-header-only", HeaderOnlyLibrary}, {"spdlog", SharedLibrary},
      {"spdlog_header_only", HeaderOnlyLibrary}}},
    {"GTest", "",
     { {"Threads", HeaderOnlyLibrary}, {"gtest", SharedLibrary}, {"gtest_main", SharedLibrary}, {"gmock", SharedLibrary},
      {"gmock_main", SharedLibrary}}},
    {"Catch2", "",
     { {"Catch2", HeaderOnlyLibrary}}},
    {"benchmark", "",
     { {"Threads", HeaderOnlyLibrary}, {"benchmark", SharedLibrary}, {"benchmark_main", SharedLibrary}}},
    {"Eigen3", "",
     { {"Eigen", HeaderOnlyLibrary}}},
    {"glm", "",
     { {"glm-header-only", HeaderOnlyLibrary}, {"glm", HeaderOnlyLibrary}}},
    {"Boost", "COMPONENTS program_options",
     { {"headers", HeaderOnlyLibrary}, {"program_options", SharedLibrary}, {"container", SharedLibrary}, {"boost", HeaderOnlyLibrary},
      {"diagnostic_definitions", HeaderOnlyLibrary}, {"disable_autolinking", HeaderOnlyLibrary}, {"dynamic_linking", HeaderOnlyLibrary}}},
    {"FlatBuffers", "",
     { {"flatc", Executable}, {"flatbuffers_shared", SharedLibrary}}},
    {"absl", "",
     { {"Threads", HeaderOnlyLibrary}, {"atomic_hook", HeaderOnlyLibrary}, {"errno_saver", HeaderOnlyLibrary}, {"log_severity", SharedLibrary},
      {"no_destructor", HeaderOnlyLibrary}, {"nullability", HeaderOnlyLibrary}, {"nullability_traits_internal", HeaderOnlyLibrary}, {"raw_logging_internal", SharedLibrary},
      {"spinlock_wait", SharedLibrary}, {"config", HeaderOnlyLibrary}, {"dynamic_annotations", HeaderOnlyLibrary}, {"core_headers", HeaderOnlyLibrary},
      {"malloc_internal", SharedLibrary}, {"base_internal", HeaderOnlyLibrary}, {"base", SharedLibrary}, {"throw_delegate", SharedLibrary},
      {"exception_testing", HeaderOnlyLibrary}, {"pretty_function", HeaderOnlyLibrary}, {"exception_safety_testing", SharedLibrary}, {"atomic_hook_test_helper", SharedLibrary},
      {"spinlock_test_common", SharedLibrary}, {"endian", HeaderOnlyLibrary}, {"scoped_set_env", SharedLibrary}, {"strerror", SharedLibrary},
      {"fast_type_id", HeaderOnlyLibrary}, {"prefetch", HeaderOnlyLibrary}, {"poison", SharedLibrary}, {"tracing_internal", SharedLibrary},
      {"iterator_traits_internal", HeaderOnlyLibrary}, {"iterator_traits_test_helper_internal", HeaderOnlyLibrary}, {"algorithm", HeaderOnlyLibrary}, {"algorithm_container", HeaderOnlyLibrary},
      {"cleanup_internal", HeaderOnlyLibrary}, {"cleanup", HeaderOnlyLibrary}, {"btree", HeaderOnlyLibrary}, {"btree_test_common", HeaderOnlyLibrary},
      {"compressed_tuple", HeaderOnlyLibrary}, {"fixed_array", HeaderOnlyLibrary}, {"inlined_vector_internal", HeaderOnlyLibrary}, {"inlined_vector", HeaderOnlyLibrary},
      {"test_allocator", HeaderOnlyLibrary}, {"test_instance_tracker", SharedLibrary}, {"flat_hash_map", HeaderOnlyLibrary}, {"flat_hash_set", HeaderOnlyLibrary},
      {"node_hash_map", HeaderOnlyLibrary}, {"node_hash_set", HeaderOnlyLibrary}, {"hash_container_defaults", HeaderOnlyLibrary}, {"container_memory", HeaderOnlyLibrary},
      {"hash_function_defaults", HeaderOnlyLibrary}, {"hash_generator_testing", SharedLibrary}, {"hash_policy_testing", HeaderOnlyLibrary}, {"hash_policy_traits", HeaderOnlyLibrary},
      {"common_policy_traits", HeaderOnlyLibrary}, {"hashtablez_sampler", SharedLibrary}, {"hashtable_debug", HeaderOnlyLibrary}, {"hashtable_debug_hooks", HeaderOnlyLibrary},
      {"node_slot_policy", HeaderOnlyLibrary}, {"raw_hash_map", HeaderOnlyLibrary}, {"container_common", HeaderOnlyLibrary}, {"hashtable_control_bytes", HeaderOnlyLibrary},
      {"raw_hash_set", SharedLibrary}, {"raw_hash_set_resize_impl", HeaderOnlyLibrary}, {"layout", HeaderOnlyLibrary}, {"tracked", HeaderOnlyLibrary},
      {"unordered_map_constructor_test", HeaderOnlyLibrary}, {"unordered_map_lookup_test", HeaderOnlyLibrary}, {"unordered_map_members_test", HeaderOnlyLibrary}, {"unordered_map_modifiers_test", HeaderOnlyLibrary},
      {"unordered_set_constructor_test", HeaderOnlyLibrary}, {"unordered_set_lookup_test", HeaderOnlyLibrary}, {"unordered_set_members_test", HeaderOnlyLibrary}, {"unordered_set_modifiers_test", HeaderOnlyLibrary},
      {"heterogeneous_lookup_testing", HeaderOnlyLibrary}, {"linked_hash_set", HeaderOnlyLibrary}, {"linked_hash_map", HeaderOnlyLibrary}, {"chunked_queue", HeaderOnlyLibrary},
      {"crc_cpu_detect", SharedLibrary}, {"crc_internal", SharedLibrary}, {"crc32c", SharedLibrary}, {"non_temporal_arm_intrinsics", HeaderOnlyLibrary},
      {"non_temporal_memcpy", HeaderOnlyLibrary}, {"crc_cord_state", SharedLibrary}, {"borrowed_fixup_buffer", SharedLibrary}, {"stacktrace", SharedLibrary},
      {"symbolize", SharedLibrary}, {"examine_stack", SharedLibrary}, {"failure_signal_handler", SharedLibrary}, {"debugging_internal", SharedLibrary},
      {"demangle_internal", SharedLibrary}, {"bounded_utf8_length_sequence", HeaderOnlyLibrary}, {"decode_rust_punycode", SharedLibrary}, {"demangle_rust", SharedLibrary},
      {"utf8_for_code_point", SharedLibrary}, {"leak_check", SharedLibrary}, {"stack_consumption", SharedLibrary}, {"debugging", HeaderOnlyLibrary},
      {"flags_path_util", HeaderOnlyLibrary}, {"flags_program_name", SharedLibrary}, {"flags_config", SharedLibrary}, {"flags_marshalling", SharedLibrary},
      {"flags_commandlineflag_internal", SharedLibrary}, {"flags_commandlineflag", SharedLibrary}, {"flags_private_handle_accessor", SharedLibrary}, {"flags_reflection", SharedLibrary},
      {"flags_internal", SharedLibrary}, {"flags", HeaderOnlyLibrary}, {"flags_usage_internal", SharedLibrary}, {"flags_usage", SharedLibrary},
      {"flags_parse", SharedLibrary}, {"any_invocable", HeaderOnlyLibrary}, {"bind_front", HeaderOnlyLibrary}, {"function_ref", HeaderOnlyLibrary},
      {"overload", HeaderOnlyLibrary}, {"hash", SharedLibrary}, {"hash_testing", HeaderOnlyLibrary}, {"spy_hash_state", HeaderOnlyLibrary},
      {"city", SharedLibrary}, {"weakly_mixed_integer", HeaderOnlyLibrary}, {"log_internal_check_impl", HeaderOnlyLibrary}, {"log_internal_check_op", SharedLibrary},
      {"log_internal_conditions", SharedLibrary}, {"log_internal_config", HeaderOnlyLibrary}, {"log_internal_flags", HeaderOnlyLibrary}, {"log_internal_format", SharedLibrary},
      {"log_internal_globals", SharedLibrary}, {"log_internal_log_impl", HeaderOnlyLibrary}, {"log_internal_proto", SharedLibrary}, {"log_internal_message", SharedLibrary},
      {"log_internal_log_sink_set", SharedLibrary}, {"log_internal_nullguard", SharedLibrary}, {"log_internal_nullstream", HeaderOnlyLibrary}, {"log_internal_strip", HeaderOnlyLibrary},
      {"log_internal_test_actions", SharedLibrary}, {"log_internal_test_helpers", SharedLibrary}, {"log_internal_test_matchers", SharedLibrary}, {"log_internal_voidify", HeaderOnlyLibrary},
      {"log_internal_append_truncated", HeaderOnlyLibrary}, {"absl_check", HeaderOnlyLibrary}, {"absl_log", HeaderOnlyLibrary}, {"check", HeaderOnlyLibrary},
      {"die_if_null", SharedLibrary}, {"log_flags", SharedLibrary}, {"log_globals", SharedLibrary}, {"log_initialize", SharedLibrary},
      {"log", HeaderOnlyLibrary}, {"log_entry", SharedLibrary}, {"log_sink", SharedLibrary}, {"log_sink_registry", HeaderOnlyLibrary},
      {"log_streamer", HeaderOnlyLibrary}, {"scoped_mock_log", SharedLibrary}, {"log_internal_structured", HeaderOnlyLibrary}, {"log_internal_structured_proto", SharedLibrary},
      {"log_structured", HeaderOnlyLibrary}, {"vlog_config_internal", SharedLibrary}, {"absl_vlog_is_on", HeaderOnlyLibrary}, {"vlog_is_on", HeaderOnlyLibrary},
      {"log_internal_fnmatch", SharedLibrary}, {"log_internal_container", HeaderOnlyLibrary}, {"memory", HeaderOnlyLibrary}, {"constexpr_testing_internal", HeaderOnlyLibrary},
      {"requires_internal", HeaderOnlyLibrary}, {"type_traits", HeaderOnlyLibrary}, {"meta", HeaderOnlyLibrary}, {"bits", HeaderOnlyLibrary},
      {"int128", SharedLibrary}, {"numeric", HeaderOnlyLibrary}, {"numeric_representation", HeaderOnlyLibrary}, {"sample_recorder", HeaderOnlyLibrary},
      {"exponential_biased", SharedLibrary}, {"periodic_sampler", SharedLibrary}, {"profile_builder", SharedLibrary}, {"hashtable_profiler", SharedLibrary},
      {"random_random", HeaderOnlyLibrary}, {"random_bit_gen_ref", HeaderOnlyLibrary}, {"random_internal_mock_helpers", HeaderOnlyLibrary}, {"random_internal_mock_overload_set", HeaderOnlyLibrary},
      {"random_mocking_bit_gen", HeaderOnlyLibrary}, {"random_distributions", SharedLibrary}, {"random_seed_gen_exception", SharedLibrary}, {"random_seed_sequences", SharedLibrary},
      {"random_internal_traits", HeaderOnlyLibrary}, {"random_internal_distribution_caller", HeaderOnlyLibrary}, {"random_internal_fast_uniform_bits", HeaderOnlyLibrary}, {"random_internal_seed_material", SharedLibrary},
      {"random_internal_entropy_pool", SharedLibrary}, {"random_internal_explicit_seed_seq", HeaderOnlyLibrary}, {"random_internal_sequence_urbg", HeaderOnlyLibrary}, {"random_internal_salted_seed_seq", HeaderOnlyLibrary},
      {"random_internal_iostream_state_saver", HeaderOnlyLibrary}, {"random_internal_generate_real", HeaderOnlyLibrary}, {"random_internal_wide_multiply", HeaderOnlyLibrary}, {"random_internal_fastmath", HeaderOnlyLibrary},
      {"random_internal_nonsecure_base", HeaderOnlyLibrary}, {"random_internal_pcg_engine", HeaderOnlyLibrary}, {"random_internal_randen_engine", HeaderOnlyLibrary}, {"random_internal_platform", SharedLibrary},
      {"random_internal_randen", SharedLibrary}, {"random_internal_randen_slow", SharedLibrary}, {"random_internal_randen_hwaes", SharedLibrary}, {"random_internal_randen_hwaes_impl", SharedLibrary},
      {"random_internal_distribution_test_util", SharedLibrary}, {"random_internal_uniform_helper", HeaderOnlyLibrary}, {"random_internal_mock_validators", HeaderOnlyLibrary}, {"status", SharedLibrary},
      {"statusor", SharedLibrary}, {"status_matchers", SharedLibrary}, {"string_view", HeaderOnlyLibrary}, {"strings", SharedLibrary},
      {"charset", HeaderOnlyLibrary}, {"has_ostream_operator", HeaderOnlyLibrary}, {"strings_internal", SharedLibrary}, {"strings_resize_and_overwrite", HeaderOnlyLibrary},
      {"strings_append_and_overwrite", HeaderOnlyLibrary}, {"str_format", HeaderOnlyLibrary}, {"str_format_internal", SharedLibrary}, {"pow10_helper", SharedLibrary},
      {"cord_internal", SharedLibrary}, {"cordz_update_tracker", HeaderOnlyLibrary}, {"cordz_functions", SharedLibrary}, {"cordz_statistics", HeaderOnlyLibrary},
      {"cordz_handle", SharedLibrary}, {"cordz_info", SharedLibrary}, {"cordz_sample_token", SharedLibrary}, {"cordz_update_scope", HeaderOnlyLibrary},
      {"cord", SharedLibrary}, {"cord_rep_test_util", HeaderOnlyLibrary}, {"cord_test_helpers", HeaderOnlyLibrary}, {"cordz_test_helpers", HeaderOnlyLibrary},
      {"generic_printer_internal", SharedLibrary}, {"graphcycles_internal", SharedLibrary}, {"kernel_timeout_internal", SharedLibrary}, {"synchronization", SharedLibrary},
      {"thread_pool", HeaderOnlyLibrary}, {"per_thread_sem_test_common", SharedLibrary}, {"time", SharedLibrary}, {"civil_time", SharedLibrary},
      {"time_zone", SharedLibrary}, {"time_internal_test_util", SharedLibrary}, {"any", HeaderOnlyLibrary}, {"span", HeaderOnlyLibrary},
      {"optional", HeaderOnlyLibrary}, {"variant", HeaderOnlyLibrary}, {"compare", HeaderOnlyLibrary}, {"bad_any_cast", HeaderOnlyLibrary},
      {"bad_optional_access", HeaderOnlyLibrary}, {"bad_variant_access", HeaderOnlyLibrary}, {"utility", HeaderOnlyLibrary}},
     {{"GTest", "GTest"}}},
    {"gRPC", "",
     { {"ZLIB", SharedLibrary}, {"Threads", HeaderOnlyLibrary}, {"libprotobuf", SharedLibrary}, {"libprotoc", SharedLibrary},
      {"protoc", Executable}, {"Crypto", SharedLibrary}, {"SSL", SharedLibrary}, {"applink", HeaderOnlyLibrary},
      {"cares", SharedLibrary}, {"cares_shared", HeaderOnlyLibrary}, {"atomic_hook", HeaderOnlyLibrary}, {"errno_saver", HeaderOnlyLibrary},
      {"log_severity", SharedLibrary}, {"no_destructor", HeaderOnlyLibrary}, {"nullability", HeaderOnlyLibrary}, {"nullability_traits_internal", HeaderOnlyLibrary},
      {"raw_logging_internal", SharedLibrary}, {"spinlock_wait", SharedLibrary}, {"config", HeaderOnlyLibrary}, {"dynamic_annotations", HeaderOnlyLibrary},
      {"core_headers", HeaderOnlyLibrary}, {"malloc_internal", SharedLibrary}, {"base_internal", HeaderOnlyLibrary}, {"base", SharedLibrary},
      {"throw_delegate", SharedLibrary}, {"exception_testing", HeaderOnlyLibrary}, {"pretty_function", HeaderOnlyLibrary}, {"exception_safety_testing", SharedLibrary},
      {"atomic_hook_test_helper", SharedLibrary}, {"spinlock_test_common", SharedLibrary}, {"endian", HeaderOnlyLibrary}, {"scoped_set_env", SharedLibrary},
      {"strerror", SharedLibrary}, {"fast_type_id", HeaderOnlyLibrary}, {"prefetch", HeaderOnlyLibrary}, {"poison", SharedLibrary},
      {"tracing_internal", SharedLibrary}, {"iterator_traits_internal", HeaderOnlyLibrary}, {"iterator_traits_test_helper_internal", HeaderOnlyLibrary}, {"algorithm", HeaderOnlyLibrary},
      {"algorithm_container", HeaderOnlyLibrary}, {"cleanup_internal", HeaderOnlyLibrary}, {"cleanup", HeaderOnlyLibrary}, {"btree", HeaderOnlyLibrary},
      {"btree_test_common", HeaderOnlyLibrary}, {"compressed_tuple", HeaderOnlyLibrary}, {"fixed_array", HeaderOnlyLibrary}, {"inlined_vector_internal", HeaderOnlyLibrary},
      {"inlined_vector", HeaderOnlyLibrary}, {"test_allocator", HeaderOnlyLibrary}, {"test_instance_tracker", SharedLibrary}, {"flat_hash_map", HeaderOnlyLibrary},
      {"flat_hash_set", HeaderOnlyLibrary}, {"node_hash_map", HeaderOnlyLibrary}, {"node_hash_set", HeaderOnlyLibrary}, {"hash_container_defaults", HeaderOnlyLibrary},
      {"container_memory", HeaderOnlyLibrary}, {"hash_function_defaults", HeaderOnlyLibrary}, {"hash_generator_testing", SharedLibrary}, {"hash_policy_testing", HeaderOnlyLibrary},
      {"hash_policy_traits", HeaderOnlyLibrary}, {"common_policy_traits", HeaderOnlyLibrary}, {"hashtablez_sampler", SharedLibrary}, {"hashtable_debug", HeaderOnlyLibrary},
      {"hashtable_debug_hooks", HeaderOnlyLibrary}, {"node_slot_policy", HeaderOnlyLibrary}, {"raw_hash_map", HeaderOnlyLibrary}, {"container_common", HeaderOnlyLibrary},
      {"hashtable_control_bytes", HeaderOnlyLibrary}, {"raw_hash_set", SharedLibrary}, {"raw_hash_set_resize_impl", HeaderOnlyLibrary}, {"layout", HeaderOnlyLibrary},
      {"tracked", HeaderOnlyLibrary}, {"unordered_map_constructor_test", HeaderOnlyLibrary}, {"unordered_map_lookup_test", HeaderOnlyLibrary}, {"unordered_map_members_test", HeaderOnlyLibrary},
      {"unordered_map_modifiers_test", HeaderOnlyLibrary}, {"unordered_set_constructor_test", HeaderOnlyLibrary}, {"unordered_set_lookup_test", HeaderOnlyLibrary}, {"unordered_set_members_test", HeaderOnlyLibrary},
      {"unordered_set_modifiers_test", HeaderOnlyLibrary}, {"heterogeneous_lookup_testing", HeaderOnlyLibrary}, {"linked_hash_set", HeaderOnlyLibrary}, {"linked_hash_map", HeaderOnlyLibrary},
      {"chunked_queue", HeaderOnlyLibrary}, {"crc_cpu_detect", SharedLibrary}, {"crc_internal", SharedLibrary}, {"crc32c", SharedLibrary},
      {"non_temporal_arm_intrinsics", HeaderOnlyLibrary}, {"non_temporal_memcpy", HeaderOnlyLibrary}, {"crc_cord_state", SharedLibrary}, {"borrowed_fixup_buffer", SharedLibrary},
      {"stacktrace", SharedLibrary}, {"symbolize", SharedLibrary}, {"examine_stack", SharedLibrary}, {"failure_signal_handler", SharedLibrary},
      {"debugging_internal", SharedLibrary}, {"demangle_internal", SharedLibrary}, {"bounded_utf8_length_sequence", HeaderOnlyLibrary}, {"decode_rust_punycode", SharedLibrary},
      {"demangle_rust", SharedLibrary}, {"utf8_for_code_point", SharedLibrary}, {"leak_check", SharedLibrary}, {"stack_consumption", SharedLibrary},
      {"debugging", HeaderOnlyLibrary}, {"flags_path_util", HeaderOnlyLibrary}, {"flags_program_name", SharedLibrary}, {"flags_config", SharedLibrary},
      {"flags_marshalling", SharedLibrary}, {"flags_commandlineflag_internal", SharedLibrary}, {"flags_commandlineflag", SharedLibrary}, {"flags_private_handle_accessor", SharedLibrary},
      {"flags_reflection", SharedLibrary}, {"flags_internal", SharedLibrary}, {"flags", HeaderOnlyLibrary}, {"flags_usage_internal", SharedLibrary},
      {"flags_usage", SharedLibrary}, {"flags_parse", SharedLibrary}, {"any_invocable", HeaderOnlyLibrary}, {"bind_front", HeaderOnlyLibrary},
      {"function_ref", HeaderOnlyLibrary}, {"overload", HeaderOnlyLibrary}, {"hash", SharedLibrary}, {"hash_testing", HeaderOnlyLibrary},
      {"spy_hash_state", HeaderOnlyLibrary}, {"city", SharedLibrary}, {"weakly_mixed_integer", HeaderOnlyLibrary}, {"log_internal_check_impl", HeaderOnlyLibrary},
      {"log_internal_check_op", SharedLibrary}, {"log_internal_conditions", SharedLibrary}, {"log_internal_config", HeaderOnlyLibrary}, {"log_internal_flags", HeaderOnlyLibrary},
      {"log_internal_format", SharedLibrary}, {"log_internal_globals", SharedLibrary}, {"log_internal_log_impl", HeaderOnlyLibrary}, {"log_internal_proto", SharedLibrary},
      {"log_internal_message", SharedLibrary}, {"log_internal_log_sink_set", SharedLibrary}, {"log_internal_nullguard", SharedLibrary}, {"log_internal_nullstream", HeaderOnlyLibrary},
      {"log_internal_strip", HeaderOnlyLibrary}, {"log_internal_test_actions", SharedLibrary}, {"log_internal_test_helpers", SharedLibrary}, {"log_internal_test_matchers", SharedLibrary},
      {"log_internal_voidify", HeaderOnlyLibrary}, {"log_internal_append_truncated", HeaderOnlyLibrary}, {"absl_check", HeaderOnlyLibrary}, {"absl_log", HeaderOnlyLibrary},
      {"check", HeaderOnlyLibrary}, {"die_if_null", SharedLibrary}, {"log_flags", SharedLibrary}, {"log_globals", SharedLibrary},
      {"log_initialize", SharedLibrary}, {"log", HeaderOnlyLibrary}, {"log_entry", SharedLibrary}, {"log_sink", SharedLibrary},
      {"log_sink_registry", HeaderOnlyLibrary}, {"log_streamer", HeaderOnlyLibrary}, {"scoped_mock_log", SharedLibrary}, {"log_internal_structured", HeaderOnlyLibrary},
      {"log_internal_structured_proto", SharedLibrary}, {"log_structured", HeaderOnlyLibrary}, {"vlog_config_internal", SharedLibrary}, {"absl_vlog_is_on", HeaderOnlyLibrary},
      {"vlog_is_on", HeaderOnlyLibrary}, {"log_internal_fnmatch", SharedLibrary}, {"log_internal_container", HeaderOnlyLibrary}, {"memory", HeaderOnlyLibrary},
      {"constexpr_testing_internal", HeaderOnlyLibrary}, {"requires_internal", HeaderOnlyLibrary}, {"type_traits", HeaderOnlyLibrary}, {"meta", HeaderOnlyLibrary},
      {"bits", HeaderOnlyLibrary}, {"int128", SharedLibrary}, {"numeric", HeaderOnlyLibrary}, {"numeric_representation", HeaderOnlyLibrary},
      {"sample_recorder", HeaderOnlyLibrary}, {"exponential_biased", SharedLibrary}, {"periodic_sampler", SharedLibrary}, {"profile_builder", SharedLibrary},
      {"hashtable_profiler", SharedLibrary}, {"random_random", HeaderOnlyLibrary}, {"random_bit_gen_ref", HeaderOnlyLibrary}, {"random_internal_mock_helpers", HeaderOnlyLibrary},
      {"random_internal_mock_overload_set", HeaderOnlyLibrary}, {"random_mocking_bit_gen", HeaderOnlyLibrary}, {"random_distributions", SharedLibrary}, {"random_seed_gen_exception", SharedLibrary},
      {"random_seed_sequences", SharedLibrary}, {"random_internal_traits", HeaderOnlyLibrary}, {"random_internal_distribution_caller", HeaderOnlyLibrary}, {"random_internal_fast_uniform_bits", HeaderOnlyLibrary},
      {"random_internal_seed_material", SharedLibrary}, {"random_internal_entropy_pool", SharedLibrary}, {"random_internal_explicit_seed_seq", HeaderOnlyLibrary}, {"random_internal_sequence_urbg", HeaderOnlyLibrary},
      {"random_internal_salted_seed_seq", HeaderOnlyLibrary}, {"random_internal_iostream_state_saver", HeaderOnlyLibrary}, {"random_internal_generate_real", HeaderOnlyLibrary}, {"random_internal_wide_multiply", HeaderOnlyLibrary},
      {"random_internal_fastmath", HeaderOnlyLibrary}, {"random_internal_nonsecure_base", HeaderOnlyLibrary}, {"random_internal_pcg_engine", HeaderOnlyLibrary}, {"random_internal_randen_engine", HeaderOnlyLibrary},
      {"random_internal_platform", SharedLibrary}, {"random_internal_randen", SharedLibrary}, {"random_internal_randen_slow", SharedLibrary}, {"random_internal_randen_hwaes", SharedLibrary},
      {"random_internal_randen_hwaes_impl", SharedLibrary}, {"random_internal_distribution_test_util", SharedLibrary}, {"random_internal_uniform_helper", HeaderOnlyLibrary}, {"random_internal_mock_validators", HeaderOnlyLibrary},
      {"status", SharedLibrary}, {"statusor", SharedLibrary}, {"status_matchers", SharedLibrary}, {"string_view", HeaderOnlyLibrary},
      {"strings", SharedLibrary}, {"charset", HeaderOnlyLibrary}, {"has_ostream_operator", HeaderOnlyLibrary}, {"strings_internal", SharedLibrary},
      {"strings_resize_and_overwrite", HeaderOnlyLibrary}, {"strings_append_and_overwrite", HeaderOnlyLibrary}, {"str_format", HeaderOnlyLibrary}, {"str_format_internal", SharedLibrary},
      {"pow10_helper", SharedLibrary}, {"cord_internal", SharedLibrary}, {"cordz_update_tracker", HeaderOnlyLibrary}, {"cordz_functions", SharedLibrary},
      {"cordz_statistics", HeaderOnlyLibrary}, {"cordz_handle", SharedLibrary}, {"cordz_info", SharedLibrary}, {"cordz_sample_token", SharedLibrary},
      {"cordz_update_scope", HeaderOnlyLibrary}, {"cord", SharedLibrary}, {"cord_rep_test_util", HeaderOnlyLibrary}, {"cord_test_helpers", HeaderOnlyLibrary},
      {"cordz_test_helpers", HeaderOnlyLibrary}, {"generic_printer_internal", SharedLibrary}, {"graphcycles_internal", SharedLibrary}, {"kernel_timeout_internal", SharedLibrary},
      {"synchronization", SharedLibrary}, {"thread_pool", HeaderOnlyLibrary}, {"per_thread_sem_test_common", SharedLibrary}, {"time", SharedLibrary},
      {"civil_time", SharedLibrary}, {"time_zone", SharedLibrary}, {"time_internal_test_util", SharedLibrary}, {"any", HeaderOnlyLibrary},
      {"span", HeaderOnlyLibrary}, {"optional", HeaderOnlyLibrary}, {"variant", HeaderOnlyLibrary}, {"compare", HeaderOnlyLibrary},
      {"bad_any_cast", HeaderOnlyLibrary}, {"bad_optional_access", HeaderOnlyLibrary}, {"bad_variant_access", HeaderOnlyLibrary}, {"utility", HeaderOnlyLibrary},
      {"uc", SharedLibrary}, {"re2", SharedLibrary}, {"address_sorting", SharedLibrary}, {"gpr", SharedLibrary},
      {"grpc", SharedLibrary}, {"grpc_unsecure", SharedLibrary}, {"grpc++", SharedLibrary}, {"grpc++_alts", SharedLibrary},
      {"grpc++_error_details", SharedLibrary}, {"grpc++_reflection", SharedLibrary}, {"grpc++_unsecure", SharedLibrary}, {"grpc_plugin_support", SharedLibrary},
      {"grpcpp_channelz", SharedLibrary}, {"upb", SharedLibrary}, {"grpc_cpp_plugin", Executable}, {"grpc_csharp_plugin", Executable},
      {"grpc_node_plugin", Executable}, {"grpc_objective_c_plugin", Executable}, {"grpc_php_plugin", Executable}, {"grpc_python_plugin", Executable},
      {"grpc_ruby_plugin", Executable}},
     {{"GTest", "GTest"}}},
    {"SFML", "COMPONENTS window",
     { {"sfml-system", SharedLibrary}, {"sfml-window", SharedLibrary}, {"X11", HeaderOnlyLibrary}, {"OpenGL", HeaderOnlyLibrary},
      {"UDev", HeaderOnlyLibrary}, {"sfml-network", SharedLibrary}, {"sfml-graphics", SharedLibrary}, {"Freetype", HeaderOnlyLibrary},
      {"VORBIS", HeaderOnlyLibrary}, {"FLAC", HeaderOnlyLibrary}, {"sfml-audio", SharedLibrary}}},
    {"glfw3", "",
     { {"Threads", HeaderOnlyLibrary}, {"glfw", SharedLibrary}}},
    {"raylib", "",
     { {"raylib", SharedLibrary}}},
    {"box2d", "",
     { {"box2d", SharedLibrary}}},
    {"Qt6", "COMPONENTS Core",
     { {"Platform", HeaderOnlyLibrary}, {"GlobalConfig", HeaderOnlyLibrary}, {"GlobalConfigPrivate", HeaderOnlyLibrary}, {"PlatformCommonInternal", HeaderOnlyLibrary},
      {"PlatformModuleInternal", HeaderOnlyLibrary}, {"PlatformPluginInternal", HeaderOnlyLibrary}, {"PlatformAppInternal", HeaderOnlyLibrary}, {"PlatformToolInternal", HeaderOnlyLibrary},
      {"PlatformExampleInternal", HeaderOnlyLibrary}, {"Threads", HeaderOnlyLibrary}, {"WrapAtomic", HeaderOnlyLibrary}, {"syncqt", Executable},
      {"moc", Executable}, {"rcc", Executable}, {"tracepointgen", Executable}, {"tracegen", Executable},
      {"cmake_automoc_parser", Executable}, {"qlalr", Executable}, {"qtpaths", Executable}, {"androiddeployqt", Executable},
      {"androidtestrunner", Executable}, {"wasmdeployqt", Executable}, {"qmake", Executable}, {"Core", SharedLibrary}}},
    {"OpenCV", "",
     { {"opencv_core", SharedLibrary}, {"opencv_flann", SharedLibrary}, {"opencv_hdf", SharedLibrary}, {"opencv_imgproc", SharedLibrary},
      {"opencv_intensity_transform", SharedLibrary}, {"opencv_ml", SharedLibrary}, {"opencv_phase_unwrapping", SharedLibrary}, {"opencv_photo", SharedLibrary},
      {"opencv_plot", SharedLibrary}, {"opencv_quality", SharedLibrary}, {"opencv_reg", SharedLibrary}, {"opencv_signal", SharedLibrary},
      {"opencv_surface_matching", SharedLibrary}, {"opencv_viz", SharedLibrary}, {"opencv_xphoto", SharedLibrary}, {"opencv_alphamat", SharedLibrary},
      {"opencv_dnn", SharedLibrary}, {"opencv_dnn_superres", SharedLibrary}, {"opencv_features2d", SharedLibrary}, {"opencv_freetype", SharedLibrary},
      {"opencv_fuzzy", SharedLibrary}, {"opencv_hfs", SharedLibrary}, {"opencv_img_hash", SharedLibrary}, {"opencv_imgcodecs", SharedLibrary},
      {"opencv_line_descriptor", SharedLibrary}, {"opencv_saliency", SharedLibrary}, {"opencv_text", SharedLibrary}, {"opencv_videoio", SharedLibrary},
      {"opencv_calib3d", SharedLibrary}, {"opencv_cvv", SharedLibrary}, {"opencv_datasets", SharedLibrary}, {"opencv_highgui", SharedLibrary},
      {"opencv_mcc", SharedLibrary}, {"opencv_objdetect", SharedLibrary}, {"opencv_rapid", SharedLibrary}, {"opencv_rgbd", SharedLibrary},
      {"opencv_shape", SharedLibrary}, {"opencv_stitching", SharedLibrary}, {"opencv_structured_light", SharedLibrary}, {"opencv_video", SharedLibrary},
      {"opencv_videostab", SharedLibrary}, {"opencv_wechat_qrcode", SharedLibrary}, {"opencv_ximgproc", SharedLibrary}, {"opencv_xobjdetect", SharedLibrary},
      {"opencv_aruco", SharedLibrary}, {"opencv_bgsegm", SharedLibrary}, {"opencv_bioinspired", SharedLibrary}, {"opencv_ccalib", SharedLibrary},
      {"opencv_dnn_objdetect", SharedLibrary}, {"opencv_dpm", SharedLibrary}, {"opencv_face", SharedLibrary}, {"opencv_gapi", SharedLibrary},
      {"opencv_optflow", SharedLibrary}, {"opencv_superres", SharedLibrary}, {"opencv_tracking", SharedLibrary}, {"opencv_stereo", SharedLibrary}}},
    {"Poco", "COMPONENTS Foundation Net",
     { {"ZLIB", SharedLibrary}, {"Pcre2", SharedLibrary}, {"Utf8Proc", SharedLibrary}, {"Foundation", SharedLibrary},
      {"Net", SharedLibrary}}},
    {"TBB", "",
     { {"tbb", SharedLibrary}, {"tbbmalloc", SharedLibrary}, {"tbbmalloc_proxy", SharedLibrary}, {"tbbbind_2_5", SharedLibrary},
      {"irml", SharedLibrary}},
     {{"hwloc", "PkgConfig"}}},
    {"LLVM", "",
     { {"ffi", SharedLibrary}, {"LibEdit", HeaderOnlyLibrary}, {"ZLIB", SharedLibrary}, {"libzstd_shared", SharedLibrary},
      {"LibXml2", SharedLibrary}, {"xmllint", Executable}, {"LLVMDemangle", StaticLibrary}, {"LLVMSupportLSP", StaticLibrary},
      {"LLVMSupport", StaticLibrary}, {"LLVMTableGen", StaticLibrary}, {"LLVMTableGenBasic", StaticLibrary}, {"LLVMTableGenCommon", StaticLibrary},
      {"llvm-tblgen", Executable}, {"LLVMABI", StaticLibrary}, {"LLVMCore", StaticLibrary}, {"LLVMFuzzerCLI", StaticLibrary},
      {"LLVMFuzzMutate", StaticLibrary}, {"LLVMFileCheck", StaticLibrary}, {"LLVMInterfaceStub", StaticLibrary}, {"LLVMIRPrinter", StaticLibrary},
      {"LLVMIRReader", StaticLibrary}, {"LLVMCAS", StaticLibrary}, {"LLVMCGData", StaticLibrary}, {"LLVMCodeGen", StaticLibrary},
      {"LLVMSelectionDAG", StaticLibrary}, {"LLVMAsmPrinter", StaticLibrary}, {"LLVMMIRParser", StaticLibrary}, {"LLVMGlobalISel", StaticLibrary},
      {"LLVMCodeGenTypes", StaticLibrary}, {"LLVMBinaryFormat", StaticLibrary}, {"LLVMBitReader", StaticLibrary}, {"LLVMBitWriter", StaticLibrary},
      {"LLVMBitstreamReader", StaticLibrary}, {"LLVMDWARFLinker", StaticLibrary}, {"LLVMDWARFLinkerClassic", StaticLibrary}, {"LLVMDWARFLinkerParallel", StaticLibrary},
      {"LLVMExtensions", StaticLibrary}, {"LLVMFrontendAtomic", StaticLibrary}, {"LLVMFrontendDirective", StaticLibrary}, {"LLVMFrontendDriver", StaticLibrary},
      {"LLVMFrontendHLSL", StaticLibrary}, {"LLVMFrontendOpenACC", StaticLibrary}, {"LLVMFrontendOpenMP", StaticLibrary}, {"LLVMFrontendOffloading", StaticLibrary},
      {"LLVMTransformUtils", StaticLibrary}, {"LLVMInstrumentation", StaticLibrary}, {"LLVMAggressiveInstCombine", StaticLibrary}, {"LLVMInstCombine", StaticLibrary},
      {"LLVMScalarOpts", StaticLibrary}, {"LLVMipo", StaticLibrary}, {"LLVMVectorize", StaticLibrary}, {"LLVMObjCARCOpts", StaticLibrary},
      {"LLVMCoroutines", StaticLibrary}, {"LLVMCFGuard", StaticLibrary}, {"LLVMHipStdPar", StaticLibrary}, {"LLVMLinker", StaticLibrary},
      {"LLVMAnalysis", StaticLibrary}, {"LLVMDTLTO", StaticLibrary}, {"LLVMLTO", StaticLibrary}, {"LLVMMC", StaticLibrary},
      {"LLVMMCParser", StaticLibrary}, {"LLVMMCDisassembler", StaticLibrary}, {"LLVMMCA", StaticLibrary}, {"LLVMObjCopy", StaticLibrary},
      {"LLVMObject", StaticLibrary}, {"LLVMObjectYAML", StaticLibrary}, {"LLVMOption", StaticLibrary}, {"LLVMRemarks", StaticLibrary},
      {"LLVMDebuginfod", StaticLibrary}, {"LLVMDebugInfoDWARFLowLevel", StaticLibrary}, {"LLVMDebugInfoDWARF", StaticLibrary}, {"LLVMDebugInfoGSYM", StaticLibrary},
      {"LLVMDebugInfoLogicalView", StaticLibrary}, {"LLVMDebugInfoMSF", StaticLibrary}, {"LLVMDebugInfoCodeView", StaticLibrary}, {"LLVMDebugInfoPDB", StaticLibrary},
      {"LLVMSymbolize", StaticLibrary}, {"LLVMDebugInfoBTF", StaticLibrary}, {"LLVMDWARFCFIChecker", StaticLibrary}, {"LLVMDWP", StaticLibrary},
      {"LLVMExecutionEngine", StaticLibrary}, {"LLVMInterpreter", StaticLibrary}, {"LLVMJITLink", StaticLibrary}, {"LLVMMCJIT", StaticLibrary},
      {"LLVMOrcJIT", StaticLibrary}, {"LLVMOrcDebugging", StaticLibrary}, {"LLVMOrcShared", StaticLibrary}, {"LLVMOrcTargetProcess", StaticLibrary},
      {"LLVMRuntimeDyld", StaticLibrary}, {"LLVMPerfJITEvents", StaticLibrary}, {"LLVMTarget", StaticLibrary}, {"LLVMAArch64CodeGen", StaticLibrary},
      {"LLVMAArch64AsmParser", StaticLibrary}, {"LLVMAArch64Disassembler", StaticLibrary}, {"LLVMAArch64Desc", StaticLibrary}, {"LLVMAArch64Info", StaticLibrary},
      {"LLVMAArch64Utils", StaticLibrary}, {"LLVMAMDGPUCodeGen", StaticLibrary}, {"LLVMAMDGPUAsmParser", StaticLibrary}, {"LLVMAMDGPUDisassembler", StaticLibrary},
      {"LLVMAMDGPUTargetMCA", StaticLibrary}, {"LLVMAMDGPUDesc", StaticLibrary}, {"LLVMAMDGPUInfo", StaticLibrary}, {"LLVMAMDGPUUtils", StaticLibrary},
      {"LLVMARMCodeGen", StaticLibrary}, {"LLVMARMAsmParser", StaticLibrary}, {"LLVMARMDisassembler", StaticLibrary}, {"LLVMARMDesc", StaticLibrary},
      {"LLVMARMInfo", StaticLibrary}, {"LLVMARMUtils", StaticLibrary}, {"LLVMAVRCodeGen", StaticLibrary}, {"LLVMAVRAsmParser", StaticLibrary},
      {"LLVMAVRDisassembler", StaticLibrary}, {"LLVMAVRDesc", StaticLibrary}, {"LLVMAVRInfo", StaticLibrary}, {"LLVMBPFCodeGen", StaticLibrary},
      {"LLVMBPFAsmParser", StaticLibrary}, {"LLVMBPFDisassembler", StaticLibrary}, {"LLVMBPFDesc", StaticLibrary}, {"LLVMBPFInfo", StaticLibrary},
      {"LLVMHexagonCodeGen", StaticLibrary}, {"LLVMHexagonAsmParser", StaticLibrary}, {"LLVMHexagonDisassembler", StaticLibrary}, {"LLVMHexagonDesc", StaticLibrary},
      {"LLVMHexagonInfo", StaticLibrary}, {"LLVMLanaiCodeGen", StaticLibrary}, {"LLVMLanaiAsmParser", StaticLibrary}, {"LLVMLanaiDisassembler", StaticLibrary},
      {"LLVMLanaiDesc", StaticLibrary}, {"LLVMLanaiInfo", StaticLibrary}, {"LLVMLoongArchCodeGen", StaticLibrary}, {"LLVMLoongArchAsmParser", StaticLibrary},
      {"LLVMLoongArchDisassembler", StaticLibrary}, {"LLVMLoongArchDesc", StaticLibrary}, {"LLVMLoongArchInfo", StaticLibrary}, {"LLVMMipsCodeGen", StaticLibrary},
      {"LLVMMipsAsmParser", StaticLibrary}, {"LLVMMipsDisassembler", StaticLibrary}, {"LLVMMipsDesc", StaticLibrary}, {"LLVMMipsInfo", StaticLibrary},
      {"LLVMMSP430CodeGen", StaticLibrary}, {"LLVMMSP430Desc", StaticLibrary}, {"LLVMMSP430Info", StaticLibrary}, {"LLVMMSP430AsmParser", StaticLibrary},
      {"LLVMMSP430Disassembler", StaticLibrary}, {"LLVMNVPTXCodeGen", StaticLibrary}, {"LLVMNVPTXDesc", StaticLibrary}, {"LLVMNVPTXInfo", StaticLibrary},
      {"LLVMPowerPCCodeGen", StaticLibrary}, {"LLVMPowerPCAsmParser", StaticLibrary}, {"LLVMPowerPCDisassembler", StaticLibrary}, {"LLVMPowerPCDesc", StaticLibrary},
      {"LLVMPowerPCInfo", StaticLibrary}, {"LLVMRISCVCodeGen", StaticLibrary}, {"LLVMRISCVAsmParser", StaticLibrary}, {"LLVMRISCVDisassembler", StaticLibrary},
      {"LLVMRISCVDesc", StaticLibrary}, {"LLVMRISCVTargetMCA", StaticLibrary}, {"LLVMRISCVInfo", StaticLibrary}, {"LLVMSparcCodeGen", StaticLibrary},
      {"LLVMSparcAsmParser", StaticLibrary}, {"LLVMSparcDisassembler", StaticLibrary}, {"LLVMSparcDesc", StaticLibrary}, {"LLVMSparcInfo", StaticLibrary},
      {"LLVMSPIRVCodeGen", StaticLibrary}, {"LLVMSPIRVDesc", StaticLibrary}, {"LLVMSPIRVInfo", StaticLibrary}, {"LLVMSPIRVAnalysis", StaticLibrary},
      {"LLVMSystemZCodeGen", StaticLibrary}, {"LLVMSystemZAsmParser", StaticLibrary}, {"LLVMSystemZDisassembler", StaticLibrary}, {"LLVMSystemZDesc", StaticLibrary},
      {"LLVMSystemZInfo", StaticLibrary}, {"LLVMVECodeGen", StaticLibrary}, {"LLVMVEAsmParser", StaticLibrary}, {"LLVMVEDisassembler", StaticLibrary},
      {"LLVMVEInfo", StaticLibrary}, {"LLVMVEDesc", StaticLibrary}, {"LLVMWebAssemblyCodeGen", StaticLibrary}, {"LLVMWebAssemblyAsmParser", StaticLibrary},
      {"LLVMWebAssemblyDisassembler", StaticLibrary}, {"LLVMWebAssemblyDesc", StaticLibrary}, {"LLVMWebAssemblyInfo", StaticLibrary}, {"LLVMWebAssemblyUtils", StaticLibrary},
      {"LLVMX86CodeGen", StaticLibrary}, {"LLVMX86AsmParser", StaticLibrary}, {"LLVMX86Disassembler", StaticLibrary}, {"LLVMX86TargetMCA", StaticLibrary},
      {"LLVMX86Desc", StaticLibrary}, {"LLVMX86Info", StaticLibrary}, {"LLVMXCoreCodeGen", StaticLibrary}, {"LLVMXCoreDisassembler", StaticLibrary},
      {"LLVMXCoreDesc", StaticLibrary}, {"LLVMXCoreInfo", StaticLibrary}, {"LLVMSandboxIR", StaticLibrary}, {"LLVMAsmParser", StaticLibrary},
      {"LLVMLineEditor", StaticLibrary}, {"LLVMProfileData", StaticLibrary}, {"LLVMCoverage", StaticLibrary}, {"LLVMPasses", StaticLibrary},
      {"LLVMPlugins", StaticLibrary}, {"LLVMTargetParser", StaticLibrary}, {"LLVMTextAPI", StaticLibrary}, {"LLVMTextAPIBinaryReader", StaticLibrary},
      {"LLVMTelemetry", StaticLibrary}, {"LLVMDlltoolDriver", StaticLibrary}, {"LLVMLibDriver", StaticLibrary}, {"LLVMXRay", StaticLibrary},
      {"LLVMTestingAnnotations", StaticLibrary}, {"LLVMTestingSupport", StaticLibrary}, {"LLVMWindowsDriver", StaticLibrary}, {"LLVMWindowsManifest", StaticLibrary},
      {"FileCheck", Executable}, {"llvm-PerfectShuffle", Executable}, {"count", Executable}, {"not", Executable},
      {"UnicodeNameMappingGenerator", Executable}, {"yaml-bench", Executable}, {"split-file", Executable}, {"llvm-test-mustache-spec", Executable},
      {"llvm_gtest", StaticLibrary}, {"llvm_gtest_main", StaticLibrary}, {"LTO", SharedLibrary}, {"LLVMgold", SharedLibrary},
      {"llvm-ar", Executable}, {"llvm-config", Executable}, {"llvm-ctxprof-util", Executable}, {"llvm-lto", Executable},
      {"llvm-profdata", Executable}, {"bugpoint", Executable}, {"dsymutil", Executable}, {"llc", Executable},
      {"lli-child-target", Executable}, {"lli", Executable}, {"llvm-as", Executable}, {"llvm-bcanalyzer", Executable},
      {"llvm-c-test", Executable}, {"llvm-cas", Executable}, {"llvm-cat", Executable}, {"llvm-cfi-verify", Executable},
      {"LLVMCFIVerify", StaticLibrary}, {"llvm-cgdata", Executable}, {"llvm-cov", Executable}, {"llvm-cvtres", Executable},
      {"llvm-cxxdump", Executable}, {"llvm-cxxfilt", Executable}, {"llvm-cxxmap", Executable}, {"llvm-debuginfo-analyzer", Executable},
      {"llvm-debuginfod", Executable}, {"llvm-debuginfod-find", Executable}, {"llvm-diff", Executable}, {"LLVMDiff", StaticLibrary},
      {"llvm-dis", Executable}, {"llvm-dwarfdump", Executable}, {"llvm-dwarfutil", Executable}, {"llvm-dwp", Executable},
      {"LLVMExegesisX86", StaticLibrary}, {"LLVMExegesisAArch64", StaticLibrary}, {"LLVMExegesisPowerPC", StaticLibrary}, {"LLVMExegesisMips", StaticLibrary},
      {"LLVMExegesisRISCV", StaticLibrary}, {"LLVMExegesis", StaticLibrary}, {"llvm-exegesis", Executable}, {"llvm-extract", Executable},
      {"llvm-gsymutil", Executable}, {"llvm-ifs", Executable}, {"llvm-ir2vec", Executable}, {"llvm-jitlink-executor", Executable},
      {"llvm-jitlink", Executable}, {"llvm-libtool-darwin", Executable}, {"llvm-link", Executable}, {"llvm-lipo", Executable},
      {"llvm-lto2", Executable}, {"llvm-mc", Executable}, {"llvm-mca", Executable}, {"llvm-ml", Executable},
      {"llvm-modextract", Executable}, {"llvm-mt", Executable}, {"llvm-nm", Executable}, {"llvm-objcopy", Executable},
      {"llvm-objdump", Executable}, {"llvm-offload-binary", Executable}, {"llvm-offload-wrapper", Executable}, {"llvm-opt-report", Executable},
      {"llvm-pdbutil", Executable}, {"llvm-profgen", Executable}, {"llvm-rc", Executable}, {"llvm-readobj", Executable},
      {"llvm-readtapi", Executable}, {"llvm-reduce", Executable}, {"llvm-remarkutil", Executable}, {"llvm-rtdyld", Executable},
      {"LLVM", SharedLibrary}, {"llvm-sim", Executable}, {"llvm-size", Executable}, {"llvm-split", Executable},
      {"llvm-stress", Executable}, {"llvm-strings", Executable}, {"llvm-symbolizer", Executable}, {"llvm-tli-checker", Executable},
      {"llvm-undname", Executable}, {"llvm-xray", Executable}, {"obj2yaml", Executable}, {"LLVMOptDriver", StaticLibrary},
      {"opt", Executable}, {"reduce-chunk-list", Executable}, {"Remarks", SharedLibrary}, {"sancov", Executable},
      {"sanstats", Executable}, {"verify-uselistorder", Executable}, {"yaml2obj", Executable}}},
    {"Clang", "",
     { {"ffi", SharedLibrary}, {"LibEdit", HeaderOnlyLibrary}, {"ZLIB", SharedLibrary}, {"libzstd_shared", SharedLibrary},
      {"LibXml2", SharedLibrary}, {"xmllint", Executable}, {"LLVMDemangle", StaticLibrary}, {"LLVMSupportLSP", StaticLibrary},
      {"LLVMSupport", StaticLibrary}, {"LLVMTableGen", StaticLibrary}, {"LLVMTableGenBasic", StaticLibrary}, {"LLVMTableGenCommon", StaticLibrary},
      {"llvm-tblgen", Executable}, {"LLVMABI", StaticLibrary}, {"LLVMCore", StaticLibrary}, {"LLVMFuzzerCLI", StaticLibrary},
      {"LLVMFuzzMutate", StaticLibrary}, {"LLVMFileCheck", StaticLibrary}, {"LLVMInterfaceStub", StaticLibrary}, {"LLVMIRPrinter", StaticLibrary},
      {"LLVMIRReader", StaticLibrary}, {"LLVMCAS", StaticLibrary}, {"LLVMCGData", StaticLibrary}, {"LLVMCodeGen", StaticLibrary},
      {"LLVMSelectionDAG", StaticLibrary}, {"LLVMAsmPrinter", StaticLibrary}, {"LLVMMIRParser", StaticLibrary}, {"LLVMGlobalISel", StaticLibrary},
      {"LLVMCodeGenTypes", StaticLibrary}, {"LLVMBinaryFormat", StaticLibrary}, {"LLVMBitReader", StaticLibrary}, {"LLVMBitWriter", StaticLibrary},
      {"LLVMBitstreamReader", StaticLibrary}, {"LLVMDWARFLinker", StaticLibrary}, {"LLVMDWARFLinkerClassic", StaticLibrary}, {"LLVMDWARFLinkerParallel", StaticLibrary},
      {"LLVMExtensions", StaticLibrary}, {"LLVMFrontendAtomic", StaticLibrary}, {"LLVMFrontendDirective", StaticLibrary}, {"LLVMFrontendDriver", StaticLibrary},
      {"LLVMFrontendHLSL", StaticLibrary}, {"LLVMFrontendOpenACC", StaticLibrary}, {"LLVMFrontendOpenMP", StaticLibrary}, {"LLVMFrontendOffloading", StaticLibrary},
      {"LLVMTransformUtils", StaticLibrary}, {"LLVMInstrumentation", StaticLibrary}, {"LLVMAggressiveInstCombine", StaticLibrary}, {"LLVMInstCombine", StaticLibrary},
      {"LLVMScalarOpts", StaticLibrary}, {"LLVMipo", StaticLibrary}, {"LLVMVectorize", StaticLibrary}, {"LLVMObjCARCOpts", StaticLibrary},
      {"LLVMCoroutines", StaticLibrary}, {"LLVMCFGuard", StaticLibrary}, {"LLVMHipStdPar", StaticLibrary}, {"LLVMLinker", StaticLibrary},
      {"LLVMAnalysis", StaticLibrary}, {"LLVMDTLTO", StaticLibrary}, {"LLVMLTO", StaticLibrary}, {"LLVMMC", StaticLibrary},
      {"LLVMMCParser", StaticLibrary}, {"LLVMMCDisassembler", StaticLibrary}, {"LLVMMCA", StaticLibrary}, {"LLVMObjCopy", StaticLibrary},
      {"LLVMObject", StaticLibrary}, {"LLVMObjectYAML", StaticLibrary}, {"LLVMOption", StaticLibrary}, {"LLVMRemarks", StaticLibrary},
      {"LLVMDebuginfod", StaticLibrary}, {"LLVMDebugInfoDWARFLowLevel", StaticLibrary}, {"LLVMDebugInfoDWARF", StaticLibrary}, {"LLVMDebugInfoGSYM", StaticLibrary},
      {"LLVMDebugInfoLogicalView", StaticLibrary}, {"LLVMDebugInfoMSF", StaticLibrary}, {"LLVMDebugInfoCodeView", StaticLibrary}, {"LLVMDebugInfoPDB", StaticLibrary},
      {"LLVMSymbolize", StaticLibrary}, {"LLVMDebugInfoBTF", StaticLibrary}, {"LLVMDWARFCFIChecker", StaticLibrary}, {"LLVMDWP", StaticLibrary},
      {"LLVMExecutionEngine", StaticLibrary}, {"LLVMInterpreter", StaticLibrary}, {"LLVMJITLink", StaticLibrary}, {"LLVMMCJIT", StaticLibrary},
      {"LLVMOrcJIT", StaticLibrary}, {"LLVMOrcDebugging", StaticLibrary}, {"LLVMOrcShared", StaticLibrary}, {"LLVMOrcTargetProcess", StaticLibrary},
      {"LLVMRuntimeDyld", StaticLibrary}, {"LLVMPerfJITEvents", StaticLibrary}, {"LLVMTarget", StaticLibrary}, {"LLVMAArch64CodeGen", StaticLibrary},
      {"LLVMAArch64AsmParser", StaticLibrary}, {"LLVMAArch64Disassembler", StaticLibrary}, {"LLVMAArch64Desc", StaticLibrary}, {"LLVMAArch64Info", StaticLibrary},
      {"LLVMAArch64Utils", StaticLibrary}, {"LLVMAMDGPUCodeGen", StaticLibrary}, {"LLVMAMDGPUAsmParser", StaticLibrary}, {"LLVMAMDGPUDisassembler", StaticLibrary},
      {"LLVMAMDGPUTargetMCA", StaticLibrary}, {"LLVMAMDGPUDesc", StaticLibrary}, {"LLVMAMDGPUInfo", StaticLibrary}, {"LLVMAMDGPUUtils", StaticLibrary},
      {"LLVMARMCodeGen", StaticLibrary}, {"LLVMARMAsmParser", StaticLibrary}, {"LLVMARMDisassembler", StaticLibrary}, {"LLVMARMDesc", StaticLibrary},
      {"LLVMARMInfo", StaticLibrary}, {"LLVMARMUtils", StaticLibrary}, {"LLVMAVRCodeGen", StaticLibrary}, {"LLVMAVRAsmParser", StaticLibrary},
      {"LLVMAVRDisassembler", StaticLibrary}, {"LLVMAVRDesc", StaticLibrary}, {"LLVMAVRInfo", StaticLibrary}, {"LLVMBPFCodeGen", StaticLibrary},
      {"LLVMBPFAsmParser", StaticLibrary}, {"LLVMBPFDisassembler", StaticLibrary}, {"LLVMBPFDesc", StaticLibrary}, {"LLVMBPFInfo", StaticLibrary},
      {"LLVMHexagonCodeGen", StaticLibrary}, {"LLVMHexagonAsmParser", StaticLibrary}, {"LLVMHexagonDisassembler", StaticLibrary}, {"LLVMHexagonDesc", StaticLibrary},
      {"LLVMHexagonInfo", StaticLibrary}, {"LLVMLanaiCodeGen", StaticLibrary}, {"LLVMLanaiAsmParser", StaticLibrary}, {"LLVMLanaiDisassembler", StaticLibrary},
      {"LLVMLanaiDesc", StaticLibrary}, {"LLVMLanaiInfo", StaticLibrary}, {"LLVMLoongArchCodeGen", StaticLibrary}, {"LLVMLoongArchAsmParser", StaticLibrary},
      {"LLVMLoongArchDisassembler", StaticLibrary}, {"LLVMLoongArchDesc", StaticLibrary}, {"LLVMLoongArchInfo", StaticLibrary}, {"LLVMMipsCodeGen", StaticLibrary},
      {"LLVMMipsAsmParser", StaticLibrary}, {"LLVMMipsDisassembler", StaticLibrary}, {"LLVMMipsDesc", StaticLibrary}, {"LLVMMipsInfo", StaticLibrary},
      {"LLVMMSP430CodeGen", StaticLibrary}, {"LLVMMSP430Desc", StaticLibrary}, {"LLVMMSP430Info", StaticLibrary}, {"LLVMMSP430AsmParser", StaticLibrary},
      {"LLVMMSP430Disassembler", StaticLibrary}, {"LLVMNVPTXCodeGen", StaticLibrary}, {"LLVMNVPTXDesc", StaticLibrary}, {"LLVMNVPTXInfo", StaticLibrary},
      {"LLVMPowerPCCodeGen", StaticLibrary}, {"LLVMPowerPCAsmParser", StaticLibrary}, {"LLVMPowerPCDisassembler", StaticLibrary}, {"LLVMPowerPCDesc", StaticLibrary},
      {"LLVMPowerPCInfo", StaticLibrary}, {"LLVMRISCVCodeGen", StaticLibrary}, {"LLVMRISCVAsmParser", StaticLibrary}, {"LLVMRISCVDisassembler", StaticLibrary},
      {"LLVMRISCVDesc", StaticLibrary}, {"LLVMRISCVTargetMCA", StaticLibrary}, {"LLVMRISCVInfo", StaticLibrary}, {"LLVMSparcCodeGen", StaticLibrary},
      {"LLVMSparcAsmParser", StaticLibrary}, {"LLVMSparcDisassembler", StaticLibrary}, {"LLVMSparcDesc", StaticLibrary}, {"LLVMSparcInfo", StaticLibrary},
      {"LLVMSPIRVCodeGen", StaticLibrary}, {"LLVMSPIRVDesc", StaticLibrary}, {"LLVMSPIRVInfo", StaticLibrary}, {"LLVMSPIRVAnalysis", StaticLibrary},
      {"LLVMSystemZCodeGen", StaticLibrary}, {"LLVMSystemZAsmParser", StaticLibrary}, {"LLVMSystemZDisassembler", StaticLibrary}, {"LLVMSystemZDesc", StaticLibrary},
      {"LLVMSystemZInfo", StaticLibrary}, {"LLVMVECodeGen", StaticLibrary}, {"LLVMVEAsmParser", StaticLibrary}, {"LLVMVEDisassembler", StaticLibrary},
      {"LLVMVEInfo", StaticLibrary}, {"LLVMVEDesc", StaticLibrary}, {"LLVMWebAssemblyCodeGen", StaticLibrary}, {"LLVMWebAssemblyAsmParser", StaticLibrary},
      {"LLVMWebAssemblyDisassembler", StaticLibrary}, {"LLVMWebAssemblyDesc", StaticLibrary}, {"LLVMWebAssemblyInfo", StaticLibrary}, {"LLVMWebAssemblyUtils", StaticLibrary},
      {"LLVMX86CodeGen", StaticLibrary}, {"LLVMX86AsmParser", StaticLibrary}, {"LLVMX86Disassembler", StaticLibrary}, {"LLVMX86TargetMCA", StaticLibrary},
      {"LLVMX86Desc", StaticLibrary}, {"LLVMX86Info", StaticLibrary}, {"LLVMXCoreCodeGen", StaticLibrary}, {"LLVMXCoreDisassembler", StaticLibrary},
      {"LLVMXCoreDesc", StaticLibrary}, {"LLVMXCoreInfo", StaticLibrary}, {"LLVMSandboxIR", StaticLibrary}, {"LLVMAsmParser", StaticLibrary},
      {"LLVMLineEditor", StaticLibrary}, {"LLVMProfileData", StaticLibrary}, {"LLVMCoverage", StaticLibrary}, {"LLVMPasses", StaticLibrary},
      {"LLVMPlugins", StaticLibrary}, {"LLVMTargetParser", StaticLibrary}, {"LLVMTextAPI", StaticLibrary}, {"LLVMTextAPIBinaryReader", StaticLibrary},
      {"LLVMTelemetry", StaticLibrary}, {"LLVMDlltoolDriver", StaticLibrary}, {"LLVMLibDriver", StaticLibrary}, {"LLVMXRay", StaticLibrary},
      {"LLVMTestingAnnotations", StaticLibrary}, {"LLVMTestingSupport", StaticLibrary}, {"LLVMWindowsDriver", StaticLibrary}, {"LLVMWindowsManifest", StaticLibrary},
      {"FileCheck", Executable}, {"llvm-PerfectShuffle", Executable}, {"count", Executable}, {"not", Executable},
      {"UnicodeNameMappingGenerator", Executable}, {"yaml-bench", Executable}, {"split-file", Executable}, {"llvm-test-mustache-spec", Executable},
      {"llvm_gtest", StaticLibrary}, {"llvm_gtest_main", StaticLibrary}, {"LTO", SharedLibrary}, {"LLVMgold", SharedLibrary},
      {"llvm-ar", Executable}, {"llvm-config", Executable}, {"llvm-ctxprof-util", Executable}, {"llvm-lto", Executable},
      {"llvm-profdata", Executable}, {"bugpoint", Executable}, {"dsymutil", Executable}, {"llc", Executable},
      {"lli-child-target", Executable}, {"lli", Executable}, {"llvm-as", Executable}, {"llvm-bcanalyzer", Executable},
      {"llvm-c-test", Executable}, {"llvm-cas", Executable}, {"llvm-cat", Executable}, {"llvm-cfi-verify", Executable},
      {"LLVMCFIVerify", StaticLibrary}, {"llvm-cgdata", Executable}, {"llvm-cov", Executable}, {"llvm-cvtres", Executable},
      {"llvm-cxxdump", Executable}, {"llvm-cxxfilt", Executable}, {"llvm-cxxmap", Executable}, {"llvm-debuginfo-analyzer", Executable},
      {"llvm-debuginfod", Executable}, {"llvm-debuginfod-find", Executable}, {"llvm-diff", Executable}, {"LLVMDiff", StaticLibrary},
      {"llvm-dis", Executable}, {"llvm-dwarfdump", Executable}, {"llvm-dwarfutil", Executable}, {"llvm-dwp", Executable},
      {"LLVMExegesisX86", StaticLibrary}, {"LLVMExegesisAArch64", StaticLibrary}, {"LLVMExegesisPowerPC", StaticLibrary}, {"LLVMExegesisMips", StaticLibrary},
      {"LLVMExegesisRISCV", StaticLibrary}, {"LLVMExegesis", StaticLibrary}, {"llvm-exegesis", Executable}, {"llvm-extract", Executable},
      {"llvm-gsymutil", Executable}, {"llvm-ifs", Executable}, {"llvm-ir2vec", Executable}, {"llvm-jitlink-executor", Executable},
      {"llvm-jitlink", Executable}, {"llvm-libtool-darwin", Executable}, {"llvm-link", Executable}, {"llvm-lipo", Executable},
      {"llvm-lto2", Executable}, {"llvm-mc", Executable}, {"llvm-mca", Executable}, {"llvm-ml", Executable},
      {"llvm-modextract", Executable}, {"llvm-mt", Executable}, {"llvm-nm", Executable}, {"llvm-objcopy", Executable},
      {"llvm-objdump", Executable}, {"llvm-offload-binary", Executable}, {"llvm-offload-wrapper", Executable}, {"llvm-opt-report", Executable},
      {"llvm-pdbutil", Executable}, {"llvm-profgen", Executable}, {"llvm-rc", Executable}, {"llvm-readobj", Executable},
      {"llvm-readtapi", Executable}, {"llvm-reduce", Executable}, {"llvm-remarkutil", Executable}, {"llvm-rtdyld", Executable},
      {"LLVM", SharedLibrary}, {"llvm-sim", Executable}, {"llvm-size", Executable}, {"llvm-split", Executable},
      {"llvm-stress", Executable}, {"llvm-strings", Executable}, {"llvm-symbolizer", Executable}, {"llvm-tli-checker", Executable},
      {"llvm-undname", Executable}, {"llvm-xray", Executable}, {"obj2yaml", Executable}, {"LLVMOptDriver", StaticLibrary},
      {"opt", Executable}, {"reduce-chunk-list", Executable}, {"Remarks", SharedLibrary}, {"sancov", Executable},
      {"sanstats", Executable}, {"verify-uselistorder", Executable}, {"yaml2obj", Executable}, {"clang-tblgen", Executable},
      {"diagtool", Executable}, {"clang", Executable}, {"clang-format", Executable}, {"clang-linker-wrapper", Executable},
      {"clang-nvlink-wrapper", Executable}, {"clang-offload-bundler", Executable}, {"clang-scan-deps", Executable}, {"clang-sycl-linker", Executable},
      {"clang-installapi", Executable}, {"clang-repl", Executable}, {"clang-refactor", Executable}, {"clang-cpp", SharedLibrary},
      {"clang-check", Executable}, {"clang-extdef-mapping", Executable}, {"clang-apply-replacements", Executable}, {"clang-reorder-fields", Executable},
      {"modularize", Executable}, {"clang-tidy", Executable}, {"clang-change-namespace", Executable}, {"clang-doc", Executable},
      {"clang-include-fixer", Executable}, {"find-all-symbols", Executable}, {"clang-move", Executable}, {"clang-query", Executable},
      {"clang-include-cleaner", Executable}, {"pp-trace", Executable}, {"clangd", Executable}, {"libclang", SharedLibrary},
      {"offload-arch", Executable}}},
    {"CGAL", "",
     { {"Data", HeaderOnlyLibrary}, {"headers", HeaderOnlyLibrary}, {"boost", HeaderOnlyLibrary}, {"diagnostic_definitions", HeaderOnlyLibrary},
      {"disable_autolinking", HeaderOnlyLibrary}, {"dynamic_linking", HeaderOnlyLibrary}, {"CGAL", HeaderOnlyLibrary}, {"Threads", HeaderOnlyLibrary},
      {"CGAL_Basic_viewer_Qt", HeaderOnlyLibrary}},
     {{"placeholder", "CGAL"}}},
    {"HPX", "",
     { {"asio", HeaderOnlyLibrary}, {"headers", HeaderOnlyLibrary}, {"boost", HeaderOnlyLibrary}, {"diagnostic_definitions", HeaderOnlyLibrary},
      {"disable_autolinking", HeaderOnlyLibrary}, {"dynamic_linking", HeaderOnlyLibrary}, {"hpx_dependencies_boost", HeaderOnlyLibrary}, {"hpx_private_flags", HeaderOnlyLibrary},
      {"hpx_public_flags", HeaderOnlyLibrary}, {"hpx_full", SharedLibrary}, {"hpx_interface", HeaderOnlyLibrary}, {"hpx_interface_wrap_main", HeaderOnlyLibrary},
      {"hpx_core", SharedLibrary}, {"hpx_affinity", HeaderOnlyLibrary}, {"hpx_algorithms", HeaderOnlyLibrary}, {"hpx_allocator_support", HeaderOnlyLibrary},
      {"hpx_asio", HeaderOnlyLibrary}, {"hpx_assertion", HeaderOnlyLibrary}, {"hpx_async_base", HeaderOnlyLibrary}, {"hpx_async_combinators", HeaderOnlyLibrary},
      {"hpx_async_local", HeaderOnlyLibrary}, {"hpx_batch_environments", HeaderOnlyLibrary}, {"hpx_cache", HeaderOnlyLibrary}, {"hpx_command_line_handling_local", HeaderOnlyLibrary},
      {"hpx_compute_local", HeaderOnlyLibrary}, {"hpx_concepts", HeaderOnlyLibrary}, {"hpx_concurrency", HeaderOnlyLibrary}, {"hpx_config", HeaderOnlyLibrary},
      {"hpx_config_registry", HeaderOnlyLibrary}, {"hpx_coroutines", HeaderOnlyLibrary}, {"hpx_datastructures", HeaderOnlyLibrary}, {"hpx_debugging", HeaderOnlyLibrary},
      {"hpx_errors", HeaderOnlyLibrary}, {"hpx_execution", HeaderOnlyLibrary}, {"hpx_execution_base", HeaderOnlyLibrary}, {"hpx_executors", HeaderOnlyLibrary},
      {"hpx_filesystem", HeaderOnlyLibrary}, {"hpx_format", HeaderOnlyLibrary}, {"hpx_functional", HeaderOnlyLibrary}, {"hpx_futures", HeaderOnlyLibrary},
      {"hpx_hardware", HeaderOnlyLibrary}, {"hpx_hashing", HeaderOnlyLibrary}, {"hpx_include_local", HeaderOnlyLibrary}, {"hpx_ini", HeaderOnlyLibrary},
      {"hpx_init_runtime_local", HeaderOnlyLibrary}, {"hpx_io_service", HeaderOnlyLibrary}, {"hpx_iterator_support", HeaderOnlyLibrary}, {"hpx_itt_notify", HeaderOnlyLibrary},
      {"hpx_lcos_local", HeaderOnlyLibrary}, {"hpx_lock_registration", HeaderOnlyLibrary}, {"hpx_logging", HeaderOnlyLibrary}, {"hpx_memory", HeaderOnlyLibrary},
      {"hpx_pack_traversal", HeaderOnlyLibrary}, {"hpx_plugin", HeaderOnlyLibrary}, {"hpx_prefix", HeaderOnlyLibrary}, {"hpx_preprocessor", HeaderOnlyLibrary},
      {"hpx_program_options", HeaderOnlyLibrary}, {"hpx_properties", HeaderOnlyLibrary}, {"hpx_resiliency", HeaderOnlyLibrary}, {"hpx_resource_partitioner", HeaderOnlyLibrary},
      {"hpx_runtime_configuration", HeaderOnlyLibrary}, {"hpx_runtime_local", HeaderOnlyLibrary}, {"hpx_schedulers", HeaderOnlyLibrary}, {"hpx_serialization", HeaderOnlyLibrary},
      {"hpx_static_reinit", HeaderOnlyLibrary}, {"hpx_string_util", HeaderOnlyLibrary}, {"hpx_synchronization", HeaderOnlyLibrary}, {"hpx_tag_invoke", HeaderOnlyLibrary},
      {"hpx_testing", HeaderOnlyLibrary}, {"hpx_thread_pool_util", HeaderOnlyLibrary}, {"hpx_thread_pools", HeaderOnlyLibrary}, {"hpx_thread_support", HeaderOnlyLibrary},
      {"hpx_threading", HeaderOnlyLibrary}, {"hpx_threading_base", HeaderOnlyLibrary}, {"hpx_threadmanager", HeaderOnlyLibrary}, {"hpx_timed_execution", HeaderOnlyLibrary},
      {"hpx_timing", HeaderOnlyLibrary}, {"hpx_topology", HeaderOnlyLibrary}, {"hpx_type_support", HeaderOnlyLibrary}, {"hpx_util", HeaderOnlyLibrary},
      {"hpx_version", HeaderOnlyLibrary}, {"hpx_actions", HeaderOnlyLibrary}, {"hpx_actions_base", HeaderOnlyLibrary}, {"hpx_agas", HeaderOnlyLibrary},
      {"hpx_agas_base", HeaderOnlyLibrary}, {"hpx_async_colocated", HeaderOnlyLibrary}, {"hpx_async_distributed", HeaderOnlyLibrary}, {"hpx_checkpoint", HeaderOnlyLibrary},
      {"hpx_checkpoint_base", HeaderOnlyLibrary}, {"hpx_collectives", HeaderOnlyLibrary}, {"hpx_command_line_handling", HeaderOnlyLibrary}, {"hpx_components", HeaderOnlyLibrary},
      {"hpx_components_base", HeaderOnlyLibrary}, {"hpx_compute", HeaderOnlyLibrary}, {"hpx_distribution_policies", HeaderOnlyLibrary}, {"hpx_executors_distributed", HeaderOnlyLibrary},
      {"hpx_include", HeaderOnlyLibrary}, {"hpx_init_runtime", HeaderOnlyLibrary}, {"hpx_lcos_distributed", HeaderOnlyLibrary}, {"hpx_naming", HeaderOnlyLibrary},
      {"hpx_naming_base", HeaderOnlyLibrary}, {"hpx_parcelport_tcp", HeaderOnlyLibrary}, {"hpx_parcelset", HeaderOnlyLibrary}, {"hpx_parcelset_base", HeaderOnlyLibrary},
      {"hpx_performance_counters", HeaderOnlyLibrary}, {"hpx_plugin_factories", HeaderOnlyLibrary}, {"hpx_resiliency_distributed", HeaderOnlyLibrary}, {"hpx_runtime_components", HeaderOnlyLibrary},
      {"hpx_runtime_distributed", HeaderOnlyLibrary}, {"hpx_segmented_algorithms", HeaderOnlyLibrary}, {"hpx_statistics", HeaderOnlyLibrary}, {"hpx_parcelports", HeaderOnlyLibrary},
      {"hpx_init", StaticLibrary}, {"hpx_wrap", StaticLibrary}, {"hpx_base_libraries", HeaderOnlyLibrary}, {"hpx", HeaderOnlyLibrary},
      {"wrap_main", HeaderOnlyLibrary}, {"plugin", HeaderOnlyLibrary}, {"component", HeaderOnlyLibrary}, {"component_storage_component", SharedLibrary},
      {"unordered_component", SharedLibrary}, {"partitioned_vector_component", SharedLibrary}, {"iostreams_component", SharedLibrary}, {"parcel_coalescing", SharedLibrary},
      {"io_counters_component", SharedLibrary}, {"memory_counters_component", SharedLibrary}, {"process_component", SharedLibrary}, {"hpx_dependencies_allocator", HeaderOnlyLibrary},
      {"Threads", HeaderOnlyLibrary}, {"hwloc", HeaderOnlyLibrary}}},
    {"OpenCASCADE", "",
     { {"TKernel", SharedLibrary}, {"TKMath", SharedLibrary}, {"TKG2d", SharedLibrary}, {"TKG3d", SharedLibrary},
      {"TKGeomBase", SharedLibrary}, {"TKBRep", SharedLibrary}, {"TKGeomAlgo", SharedLibrary}, {"TKTopAlgo", SharedLibrary},
      {"TKPrim", SharedLibrary}, {"TKBO", SharedLibrary}, {"TKShHealing", SharedLibrary}, {"TKBool", SharedLibrary},
      {"TKHLR", SharedLibrary}, {"TKFillet", SharedLibrary}, {"TKOffset", SharedLibrary}, {"TKFeat", SharedLibrary},
      {"TKMesh", SharedLibrary}, {"TKXMesh", SharedLibrary}, {"TKService", SharedLibrary}, {"TKV3d", SharedLibrary},
      {"TKOpenGl", SharedLibrary}, {"TKMeshVS", SharedLibrary}, {"TKIVtk", SharedLibrary}, {"TKCDF", SharedLibrary},
      {"TKLCAF", SharedLibrary}, {"TKCAF", SharedLibrary}, {"TKBinL", SharedLibrary}, {"TKXmlL", SharedLibrary},
      {"TKBin", SharedLibrary}, {"TKXml", SharedLibrary}, {"TKStdL", SharedLibrary}, {"TKStd", SharedLibrary},
      {"TKTObj", SharedLibrary}, {"TKBinTObj", SharedLibrary}, {"TKXmlTObj", SharedLibrary}, {"TKVCAF", SharedLibrary},
      {"TKDE", SharedLibrary}, {"TKXSBase", SharedLibrary}, {"TKDESTEP", SharedLibrary}, {"TKXCAF", SharedLibrary},
      {"TKDEIGES", SharedLibrary}, {"TKDESTL", SharedLibrary}, {"TKDEVRML", SharedLibrary}, {"TKRWMesh", SharedLibrary},
      {"TKDECascade", SharedLibrary}, {"TKBinXCAF", SharedLibrary}, {"TKXmlXCAF", SharedLibrary}, {"TKDEOBJ", SharedLibrary},
      {"TKDEGLTF", SharedLibrary}, {"TKDEPLY", SharedLibrary}, {"TKExpress", SharedLibrary}, {"TKDraw", SharedLibrary},
      {"TKTopTest", SharedLibrary}, {"TKOpenGlTest", SharedLibrary}, {"TKViewerTest", SharedLibrary}, {"TKXSDRAW", SharedLibrary},
      {"TKDCAF", SharedLibrary}, {"TKXDEDRAW", SharedLibrary}, {"TKTObjDRAW", SharedLibrary}, {"TKQADraw", SharedLibrary},
      {"TKIVtkDraw", SharedLibrary}, {"TKXSDRAWDE", SharedLibrary}, {"TKXSDRAWGLTF", SharedLibrary}, {"TKXSDRAWIGES", SharedLibrary},
      {"TKXSDRAWOBJ", SharedLibrary}, {"TKXSDRAWPLY", SharedLibrary}, {"TKXSDRAWSTEP", SharedLibrary}, {"TKXSDRAWSTL", SharedLibrary},
      {"TKXSDRAWVRML", SharedLibrary}},
     {{"VTK", "VTK"}}},
    {"ITK", "",
     { {"itkdouble-conversion", SharedLibrary}, {"itksys", SharedLibrary}, {"ITKVNLInstantiation", SharedLibrary}, {"ITKCommon", SharedLibrary},
      {"itkNetlibSlatec", SharedLibrary}, {"ITKStatistics", SharedLibrary}, {"ITKTransform", SharedLibrary}, {"ITKIOImageBase", SharedLibrary},
      {"ITKIOBMP", SharedLibrary}, {"ITKIOGDCM", SharedLibrary}, {"ITKIOGIPL", SharedLibrary}, {"ITKIOJPEG", SharedLibrary},
      {"ITKMetaIO", SharedLibrary}, {"ITKIOMeta", SharedLibrary}, {"ITKznz", SharedLibrary}, {"ITKniftiio", SharedLibrary},
      {"ITKIONIFTI", SharedLibrary}, {"ITKNrrdIO", SharedLibrary}, {"ITKIONRRD", SharedLibrary}, {"ITKIOPNG", SharedLibrary},
      {"ITKIOTIFF", SharedLibrary}, {"ITKIOVTK", SharedLibrary}, {"itkTestDriver", Executable}, {"ITKLabelMap", SharedLibrary},
      {"ITKMesh", SharedLibrary}, {"ITKSpatialObjects", SharedLibrary}, {"ITKPath", SharedLibrary}, {"ITKQuadEdgeMesh", SharedLibrary},
      {"ITKOptimizers", SharedLibrary}, {"ITKPolynomials", SharedLibrary}, {"ITKBiasCorrection", SharedLibrary}, {"ITKBioCell", SharedLibrary},
      {"ITKFFT", SharedLibrary}, {"ITKDICOMParser", SharedLibrary}, {"ITKIOXML", SharedLibrary}, {"ITKIOSpatialObjects", SharedLibrary},
      {"ITKFEM", SharedLibrary}, {"ITKgiftiio", SharedLibrary}, {"ITKIOMesh", SharedLibrary}, {"ITKIOBioRad", SharedLibrary},
      {"ITKIOBruker", SharedLibrary}, {"ITKIOCSV", SharedLibrary}, {"ITKIOIPL", SharedLibrary}, {"ITKIOGE", SharedLibrary},
      {"ITKIOSiemens", SharedLibrary}, {"ITKIOHDF5", SharedLibrary}, {"ITKIOLSM", SharedLibrary}, {"ITKIOMINC", SharedLibrary},
      {"ITKIOMRC", SharedLibrary}, {"ITKIOStimulate", SharedLibrary}, {"ITKTransformFactory", SharedLibrary}, {"ITKIOTransformBase", SharedLibrary},
      {"ITKIOTransformHDF5", SharedLibrary}, {"ITKIOTransformInsightLegacy", SharedLibrary}, {"ITKIOTransformMatlab", SharedLibrary}, {"ITKKLMRegionGrowing", SharedLibrary},
      {"ITKVTK", SharedLibrary}, {"ITKWatersheds", SharedLibrary}, {"itklbfgs", SharedLibrary}, {"ITKOptimizersv4", SharedLibrary},
      {"itkopenjpeg", SharedLibrary}, {"ITKReview", SharedLibrary}, {"ITKVideoCore", SharedLibrary}, {"ITKVideoIO", SharedLibrary},
      {"ITKVtkGlue", SharedLibrary}, {"gdcmjpeg8", SharedLibrary}, {"gdcmjpeg12", SharedLibrary}, {"gdcmjpeg16", SharedLibrary},
      {"gdcmmd5", SharedLibrary}, {"socketxx", SharedLibrary}, {"gdcmCommon", SharedLibrary}, {"gdcmDICT", SharedLibrary},
      {"gdcmDSED", SharedLibrary}, {"gdcmIOD", SharedLibrary}, {"gdcmMSFF", SharedLibrary}, {"gdcmMEXD", SharedLibrary},
      {"gdcmdump", Executable}, {"gdcmdiff", Executable}, {"gdcmraw", Executable}, {"gdcmscanner", Executable},
      {"gdcmanon", Executable}, {"gdcmclean", Executable}, {"gdcmgendir", Executable}, {"gdcmimg", Executable},
      {"gdcmconv", Executable}, {"gdcmtar", Executable}, {"gdcminfo", Executable}, {"gdcmscu", Executable},
      {"gdcmxml", Executable}, {"gdcmpap3", Executable}, {"gdcmpdf", Executable}},
     {{"VTK", "VTK"}},
     "-DITK_DIR=/usr/lib64/cmake/InsightToolkit "
                                                   "-DITKVtkGlue_LOADED=1"},
    {"Ceres", "",
     { {"Threads", HeaderOnlyLibrary}, {"BLAS", HeaderOnlyLibrary}, {"LAPACK", HeaderOnlyLibrary}, {"AMD", SharedLibrary},
      {"CAMD", SharedLibrary}, {"CCOLAMD", SharedLibrary}, {"CHOLMOD", SharedLibrary}, {"COLAMD", SharedLibrary},
      {"SPQR", SharedLibrary}, {"Config", SharedLibrary}, {"tbb", SharedLibrary}, {"tbbmalloc", SharedLibrary},
      {"tbbmalloc_proxy", SharedLibrary}, {"tbbbind_2_5", SharedLibrary}, {"irml", SharedLibrary}, {"Eigen", HeaderOnlyLibrary},
      {"gflags_shared", SharedLibrary}, {"gflags_nothreads_shared", SharedLibrary}, {"gflags", SharedLibrary}, {"glog", SharedLibrary},
      {"ceres", SharedLibrary}},
     {{"hwloc", "PkgConfig"}}},
    // Variable only config -> no targets -> we synthesize the target
    {"Bullet", "",
     { {"Bullet_libs", HeaderOnlyLibrary}}},
};
// clang-format on

// hwloc ships no CMake config; TBB's and Ceres's configs reference pkg-config's
// PkgConfig::HWLOC imported target. Fabricated from the dnf-installed files
// (lib64/libhwloc.so under /usr, includes in /usr/include — verified in container).
ThirdPartyTargetManifest fabricate_hwloc()
{
    ThirdPartyTarget *tpt = ThirdPartyTarget::make("hwloc", Directory::make("/usr"));
    zimm::SharedLibrary *hwloc =
        tpt->assume_shared_library("HWLOC", detail::RelativePath{"lib64/libhwloc.so"});
    tpt->add_public_property(IncludeProperty{Directory::make("/usr/include")});
    return {tpt, {hwloc}};
}

// CGALConfig imports CGAL::CGAL_Qt6 only when Qt6 is found in the same CMake session
// (Qt6 is not installed); all its CGAL_Qt6 creation sites are guarded, so a placeholder
// with an empty interface satisfies the CGAL::CGAL_BasicViewer_Qt link reference.
ThirdPartyTargetManifest fabricate_cgal_qt6_placeholder()
{
    ThirdPartyTarget *tpt =
        ThirdPartyTarget::make("cgal_qt6_placeholder", Directory::make("/usr"));
    zimm::HeaderOnlyLibrary *qt6 = make_header_only_library("CGAL_Qt6");
    add_dependency_rel(qt6, tpt);
    return {tpt, {qt6}};
}

// vtk-config's targets reference ~33 third-party targets it never find_package()s
// (consumer-preload convention), so find_package(VTK) cannot configure — and ITK's
// ITKVtkGlue module find_package(VTK)s internally with the same result. Hand-fabricate
// the union of the VTK targets OpenCASCADE's visualization/draw targets (TKIVtk,
// TKIVtkDraw) and ITK's imported ITKVtkGlue target reference.
ThirdPartyTargetManifest fabricate_vtk()
{
    ThirdPartyTarget *tpt = ThirdPartyTarget::make("vtk", Directory::make("/usr"));
    std::vector<Target *> targets;
    for (std::string_view name : {"CommonCore", "FiltersGeneral", "IOImage", "ImagingCore",
                                  "ImagingSources", "InteractionStyle", "RenderingCore",
                                  "RenderingFreeType", "RenderingGL2PSOpenGL2",
                                  "RenderingOpenGL2"})
        targets.push_back(tpt->assume_shared_library(
            std::string{name}, detail::RelativePath{std::format("lib64/libvtk{}.so", name)}));
    tpt->add_public_property(IncludeProperty{Directory::make("/usr/include/vtk")});
    return {tpt, std::move(targets)};
}

struct Error
{
    std::string stage;
    std::string reason;
};

const Target *resolve(std::span<const Target *> materialized, const Expected &exp)
{
    for (const Target *t : materialized)
        if (t->name() == exp.name && t->type() == exp.type) return t;
    return nullptr;
}

void check_import(const ThirdPartyTargetManifest &manifest, std::string_view cmakeName)
{
    if (!manifest.tpt())
        throw Error{.stage = "import",
                    .reason = std::format("empty manifest (configure or parse failed) — see "
                                          "from_docker/.zimm_cmake_find/{}/configure.log for "
                                          "the wrapper log of '{}' (zimm copies it out of the "
                                          "container)",
                                          cmakeName, cmakeName)};
}

void check_targets(const ThirdPartyTargetManifest &manifest, const Package &pkg)
{
    auto assumedTargets = manifest.targets();
    std::vector<std::string> missing;

    for (const Expected &exp : pkg.expected)
        if (!resolve(assumedTargets, exp))
            missing.push_back(std::format("{} ({})", exp.name, to_string(exp.type)));

    if (!missing.empty())
        throw Error{.stage = "targets",
                    .reason =
                        "missing: " + (missing | std::views::join_with(std::string_view{", "}) |
                                       std::ranges::to<std::string>())};
}

// Inverse of check_targets: every materialized target must be expected. Catches the
// strategy inventing targets the config does not expose (or name/type drift).
void check_no_extras(const ThirdPartyTargetManifest &manifest, const Package &pkg)
{
    auto assumedTargets = manifest.targets();
    std::vector<std::string> extras;

    for (const Target *t : assumedTargets)
    {
        bool known = false;
        for (const Expected &exp : pkg.expected)
            if (t->name() == exp.name && t->type() == exp.type)
            {
                known = true;
                break;
            }
        if (!known)
            extras.push_back(std::format("{} ({})", t->name(), to_string(t->type())));
    }

    if (!extras.empty())
        throw Error{.stage = "extras",
                    .reason = "materialized but not expected: " +
                              (extras | std::views::join_with(std::string_view{", "}) |
                               std::ranges::to<std::string>())};
}

void check_paths(const ThirdPartyTargetManifest &manifest, const Package &pkg)
{
    auto assumedTargets = manifest.targets();

    std::vector<std::string> badPaths;
    for (const Expected &exp : pkg.expected)
    {
        if (exp.type == HeaderOnlyLibrary) continue;

        const Target *t = resolve(assumedTargets, exp);
        const auto &assumed = t->assumed_path();

        if (!assumed || !fs::exists(assumed->path()))
            badPaths.push_back(
                std::format("{}: {}", t->name(),
                            assumed ? assumed->path().string() : std::string{"no assumed path"}));
    }

    if (!badPaths.empty())
        throw Error{.stage = "paths",
                    .reason = "missing artifacts: " +
                              (badPaths | std::views::join_with(std::string_view{", "}) |
                               std::ranges::to<std::string>())};
}

int test_packages()
{
    std::size_t passCount = 0, failCount = 0;
    std::vector<std::string> summary;

    // Manifests rows can inject: hwloc (TBB/Ceres rows), the CGAL_Qt6 placeholder (CGAL
    // row), the VTK targets OpenCASCADE's visualization targets reference (OpenCASCADE
    // row), plus every row's own manifest once found (GTest row 3 feeds absl 10/gRPC 11).
    std::map<std::string, ThirdPartyTargetManifest, std::less<>> found;
    found.emplace("hwloc", fabricate_hwloc());
    found.emplace("placeholder", fabricate_cgal_qt6_placeholder());
    found.emplace("VTK", fabricate_vtk());

    for (const Package &pkg : packages)
    {
        std::cout << "== find_package(" << pkg.cmakeName << " CONFIG REQUIRED";
        if (!pkg.cmakeArgs.empty()) std::cout << " " << pkg.cmakeArgs;
        std::cout << ")\n";

        try
        {
            std::vector<CmakeDependency> deps;
            for (const DepSpec &spec : pkg.deps)
            {
                auto it = found.find(spec.pkg);
                if (it == found.end())
                {
                    std::cout << std::format(
                        "WARN  no manifest '{}' to inject — '{}' runs uninjected\n", spec.pkg,
                        pkg.cmakeName);
                    continue;
                }
                deps.push_back({std::string{spec.ns}, it->second});
            }

            FindCmakePackageTptStrategy strategy{
                std::vector<Directory>{Directory::make("/usr")}, std::string{pkg.cmakeArgs},
                "relwithdebinfo", std::string{pkg.hints}};
            auto manifest = strategy.attempt(pkg.cmakeName, deps);
            found[std::string{pkg.cmakeName}] = manifest; // rows feed later rows

            check_import(manifest, pkg.cmakeName);
            check_targets(manifest, pkg);
            check_no_extras(manifest, pkg);
            check_paths(manifest, pkg);

            ++passCount;
            std::string expectedNames = pkg.expected | std::views::transform(&Expected::name) |
                                        std::views::join_with(std::string_view{", "}) |
                                        std::ranges::to<std::string>();
            summary.push_back(
                std::format("PASS  {:<12} {:<8} {}", pkg.cmakeName, "-", expectedNames));
        }
        catch (const Error &e)
        {
            ++failCount;
            summary.push_back(
                std::format("FAIL  {:<12} {:<8} {}", pkg.cmakeName, e.stage, e.reason));
        }
    }

    std::cout << "\n==== summary ====" << std::endl;
    for (const std::string &line : summary) std::cout << line << std::endl;
    std::cout << std::format("\n{} pass, {} fail", passCount, failCount) << std::endl;
    return failCount == 0 ? 0 : 1;
}

} // namespace

int main() { return test_packages(); }
