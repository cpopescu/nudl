#ifndef NUDL_ANALYSIS_TYPE_STORE_H__
#define NUDL_ANALYSIS_TYPE_STORE_H__

#include <memory>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "nudl/analysis/names.h"
#include "nudl/analysis/type_spec.h"
#include "nudl/proto/dsl.pb.h"

namespace nudl {
namespace analysis {

class TypeStore {
 public:
  TypeStore() {}
  virtual ~TypeStore() {}
  // Finds, and possibly binds the type.
  //  lookup_scope - the scope in which the lookup is requested.
  //  type_spec - the specification of the type.
  virtual absl::StatusOr<const TypeSpec*> FindType(
      const ScopeName& lookup_scope, const pb::TypeSpec& type_spec) = 0;

  // Finds a (base) type in this type store.
  virtual absl::StatusOr<const TypeSpec*> FindTypeByName(
      absl::string_view name) const = 0;

  // Adds a new type to the store in provided scope.
  virtual absl::StatusOr<const TypeSpec*> DeclareType(
      const ScopeName& scope_name, absl::string_view name,
      std::unique_ptr<TypeSpec> type_spec) = 0;

  // Dumps the type names in the store.
  virtual std::string DebugNames() const = 0;
};

class ScopeTypeStore;

class GlobalTypeStore : public TypeStore {
 public:
  explicit GlobalTypeStore(std::unique_ptr<TypeStore> base_store = nullptr);

  absl::StatusOr<const TypeSpec*> FindType(
      const ScopeName& lookup_scope, const pb::TypeSpec& type_spec) override;
  absl::StatusOr<const TypeSpec*> FindTypeByName(
      absl::string_view name) const override;
  absl::StatusOr<const TypeSpec*> DeclareType(
      const ScopeName& scope_name, absl::string_view name,
      std::unique_ptr<TypeSpec> type_spec) override;

  absl::Status AddScope(std::shared_ptr<ScopeName> scope_name);
  absl::optional<ScopeTypeStore*> FindStore(absl::string_view name) const;
  const TypeStore& base_store() const;
  TypeStore* mutable_base_store();

  std::string DebugNames() const override;

 protected:
  std::unique_ptr<TypeStore> base_store_;
  absl::flat_hash_map<std::string, std::unique_ptr<ScopeTypeStore>> scopes_;
};

class ScopeTypeStore : public TypeStore {
 public:
  ScopeTypeStore(std::shared_ptr<ScopeName> scope_name,
                 TypeStore* global_store);

  absl::StatusOr<const TypeSpec*> FindType(
      const ScopeName& lookup_scope, const pb::TypeSpec& type_spec) override;
  absl::StatusOr<const TypeSpec*> FindTypeByName(
      absl::string_view name) const override;
  absl::StatusOr<const TypeSpec*> DeclareType(
      const ScopeName& scope_name, absl::string_view name,
      std::unique_ptr<TypeSpec> type_spec) override;
  bool HasType(absl::string_view type_name) const;

  std::string DebugNames() const override;

 protected:
  // Used by FindTypeByName to lookup local types / defines.
  absl::StatusOr<const TypeSpec*> FindTypeLocal(const ScopeName& lookup_scope,
                                                const pb::TypeSpec& type_spec);
  // Adds a local type mapping to Any.
  absl::StatusOr<const TypeSpec*> DeclareLocalAnyType(absl::string_view name);

  std::shared_ptr<ScopeName> scope_name_;
  TypeStore* global_store_ = nullptr;

  absl::flat_hash_map<std::string, std::unique_ptr<TypeSpec>> types_;
  std::vector<std::unique_ptr<TypeSpec>> bound_types_;
};

}  // namespace analysis
}  // namespace nudl

#endif  // NUDL_ANALYSIS_TYPE_STORE_H__
