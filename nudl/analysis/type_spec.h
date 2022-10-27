#ifndef NUDL_ANALYSIS_TYPE_SPEC_H__
#define NUDL_ANALYSIS_TYPE_SPEC_H__

#include <any>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/statusor.h"
#include "absl/types/variant.h"
#include "nudl/analysis/named_object.h"

namespace nudl {
namespace analysis {

class TypeSpec;
using TypeBindingArg = absl::variant<int, const TypeSpec*>;

// The normal name store that can be used with types to hold
// fields and member functions:
class TypeMemberStore : public BaseNameStore {
 public:
  TypeMemberStore(const TypeSpec* type_spec,
                  std::shared_ptr<NameStore> ancestor);
  ~TypeMemberStore();

  // The base class for the underlying type:
  NameStore* ancestor() const;
  std::shared_ptr<NameStore> ancestor_ptr() const;

  // Usual NamedObject interface:
  pb::ObjectKind kind() const override;
  std::string full_name() const override;
  const TypeSpec* type_spec() const override;

  // Usual NameStore interface, overriden to lookup in binding_parent
  // or ancestor if not found in here:
  bool HasName(absl::string_view local_name, bool in_self_only) const override;
  absl::StatusOr<NamedObject*> GetName(absl::string_view local_name,
                                       bool in_self_only) override;
  absl::Status AddName(absl::string_view local_name,
                       NamedObject* object) override;
  absl::Status AddChildStore(absl::string_view local_name,
                             NameStore* store) override;

  // Returns a vector of stores in which order to find members of this
  // type for binding.
  std::vector<NameStore*> FindBindingOrder();
  std::vector<const NameStore*> FindConstBindingOrder() const;

  // This is the type member store for the main instance of a type.
  // E.g. for Array<Int> this would point to Array<Any> (note for now not to
  //  Array<Integral> or so, because may become complicated for multiple param
  //  types)
  // TODO(catalin): fix this maybe or see how we can alleviate the possible
  // issues.
  absl::optional<TypeMemberStore*> binding_parent() const;
  // The signature under which this type is bound to
  const std::string& binding_signature() const;
  // The children bindings of this store. E.g. for Array<Any> this would contain
  // the stores of Array<Int> etc.
  const absl::flat_hash_map<std::string, std::shared_ptr<TypeMemberStore>>&
  bound_children() const;

  // Adds a child binding, under a type signature.
  std::shared_ptr<TypeMemberStore> AddBinding(absl::string_view signature,
                                              const TypeSpec* type_spec);
  // Removes the child binding for the signature.
  void RemoveBinding(absl::string_view signature);
  // Sets up the binding_parent, which we are register to under provided
  // signature.
  void SetupBindingParent(absl::string_view signature,
                          TypeMemberStore* binding_parent);
  // Signal from binding_parent to remove itself, during destruction sequence.
  void RemoveBindingParent();

  // Adds a type that uses us as a type member store.
  void AddMemberType(const TypeSpec* member_type);
  // Removes a type that uses us as a type member store.
  void RemoveMemberType(const TypeSpec* member_type);

 protected:
  friend class TypeSpec;

  absl::Status CheckAddedObject(absl::string_view local_name,
                                NamedObject* object);

  absl::optional<const TypeSpec*> type_spec_;
  std::shared_ptr<NameStore> const ancestor_;
  absl::optional<TypeMemberStore*> binding_parent_;
  std::string binding_signature_;
  absl::flat_hash_map<std::string, std::shared_ptr<TypeMemberStore>>
      bound_children_;
  absl::flat_hash_set<const TypeSpec*> member_types_;
};

// Structure that represents a type.
//  - type_id - corresponds to the base type from which this is derived.
//  - name - the name of this type - base type name for most of the time.
//  - is_bound_type - if this type is in itself bound ie. can be instantiated
//      to an implementation type. E.g. Numeric is not bounded, but Int it is,
//      Iterable is not bound but Array is in itself (but the full type may not
//      be, if counting for parameters).
//  - ancestor - the base type this for this one. 'Any' type is at the root
//      of the entire hierarchy.
//  - parameters - type parameter for this type. Eg. for Array < T > is the T.
class TypeSpec : public NamedObject {
 public:
  TypeSpec(int type_id, absl::string_view name,
           std::shared_ptr<TypeMemberStore> type_member_store,
           bool is_bound_type = false, const TypeSpec* ancestor = nullptr,
           std::vector<const TypeSpec*> parameters = {});
  ~TypeSpec() override;

  // The unique identifier of this type.
  int type_id() const;
  // If the type in itself is bound.
  bool is_bound_type() const;
  // Base type(s) for this.
  const TypeSpec* ancestor() const;
  // Parameters that fully define the type.
  const std::vector<const TypeSpec*>& parameters() const;
  // Associated name store for members.
  TypeMemberStore* type_member_store() const;
  std::shared_ptr<TypeMemberStore> type_member_store_ptr() const;
  // Local name of the type in the store.
  const std::string& local_name() const;
  // The scope in which the type is defined:
  virtual const ScopeName& scope_name() const = 0;

  // Converts this type to a proto representation.
  virtual pb::ExpressionTypeSpec ToProto() const;

  // NamedObject interface:
  pb::ObjectKind kind() const override;
  absl::optional<NameStore*> name_store() override;

  // Sets the name of the type in the local store.
  void set_local_name(absl::string_view local_name);

  // If this type, and all its parameters are bound.
  virtual bool IsBound() const;
  // Full type specification.
  std::string full_name() const override;
  // If this type is an ancestor (maybe not direct) of type_spec.
  virtual bool IsAncestorOf(const TypeSpec& type_spec) const;
  // If the type_spec and this type are the same.
  virtual bool IsEqual(const TypeSpec& type_spec) const;
  // If this type can be converted from type_spec
  virtual bool IsConvertibleFrom(const TypeSpec& type_spec) const;

  // TODO(catalin): convert all these to optional<...>

  // If this is an iterable type, this is the type of the elements
  // that it produces in an iteration.
  // For function types it represents the return type.
  virtual const TypeSpec* ResultType() const;
  // If this type supports indexed access with [] operator, this
  // would return the type accepted for the index operation.
  virtual const TypeSpec* IndexType() const;
  // The type of the value returned by the [] operator.
  virtual const TypeSpec* IndexedType() const;
  // If this is an iterable type.
  virtual bool IsIterable() const;

  // If this is a basic type, passed by value to a function, and
  // has a local in-function value.
  bool IsBasicType() const;

  // If provided type_id is of a basic type.
  static bool IsBasicTypeId(int type_id);

  // Binds the parameters of this type to other types.
  virtual absl::StatusOr<std::unique_ptr<TypeSpec>> Bind(
      const std::vector<TypeBindingArg>& bindings) const;

  // Creates a copy of this type:
  virtual std::unique_ptr<TypeSpec> Clone() const = 0;

  // Returns a composed string from a type binding:
  static std::string TypeBindingSignature(
      const std::vector<TypeBindingArg>& type_arguments);

  // Sets the name of a type - this can be done only onece.
  absl::Status set_name(absl::string_view name);

  // Utility for bindings conversion - public for testing.
  absl::StatusOr<std::vector<const TypeSpec*>> TypesFromBindings(
      const std::vector<TypeBindingArg>& bindings, bool check_params = true,
      absl::optional<size_t> minimum_parameters = {}) const;

 protected:
  // Updates the type_member_store per provided bindings.
  absl::Status UpdateBindingStore(const std::vector<TypeBindingArg>& bindings);

  // Checks parameters of this and type_spec for ancestry.
  bool HasAncestorParameters(const TypeSpec& type_spec) const;
  // Checks parameters of this and type_spec for convertible.
  bool HasConvertibleParameters(const TypeSpec& type_spec) const;
  // If we can use the result type for comparing parameters of
  // this type to the one of type_spec.
  bool IsResultTypeComparable(const TypeSpec& type_spec) const;
  // Wraps the provided type name in the local_name_(if set):
  std::string wrap_local_name(std::string s) const;

  static int NextTypeId();
  const int type_id_;  // pb::TypeId values reserved for builtins.
  std::shared_ptr<TypeMemberStore> type_member_store_;
  const bool is_bound_type_;
  bool is_name_set_ = false;

  std::string local_name_;
  const TypeSpec* ancestor_ = nullptr;
  std::vector<const TypeSpec*> parameters_;
};

// Class helper for rebinding types. The concept here is that one would
// call ProcessType with original type specification and actual type instances,
// local names are recorded in this class.
// Afterwards, RebuildType is called with the original type instances to
// create new types bound with the call types negotiated during ProcessTypes.
// All allocated types have to be picked up after use from allocated_types
// member.
//
// This binding class accepts a partial set of parameters, checks and unifies
// the local names during ProcessType, then calls RebuildType to bind on an
// updates set of bindings.
// E.g Function<{T:Numeric}, T, T>
//  binding the first parameters to, say Int, binds the rest of T-s to Int.
// Binding with Int and Decimal may result in an error.
class LocalNamesRebinder {
 public:
  LocalNamesRebinder() {}
  absl::Status ProcessType(const TypeSpec* src_param,
                           const TypeSpec* type_spec);
  absl::StatusOr<const TypeSpec*> RebuildType(const TypeSpec* src_param,
                                              const TypeSpec* type_spec);
  // Same as above, but specifically for functions.
  absl::StatusOr<const TypeSpec*> RebuildFunctionWithComponents(
      const TypeSpec* src_param,
      const std::vector<const TypeSpec*>& type_specs);

  std::vector<std::unique_ptr<TypeSpec>> allocated_types;

 private:
  absl::Status RecordLocalName(const TypeSpec* src_param,
                               const TypeSpec* type_spec);

  absl::flat_hash_map<std::string, const TypeSpec*> local_types_;
};

}  // namespace analysis
}  // namespace nudl

#endif  // NUDL_ANALYSIS_TYPE_SPEC_H__
