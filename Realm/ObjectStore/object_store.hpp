////////////////////////////////////////////////////////////////////////////
//
// Copyright 2015 Realm Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
////////////////////////////////////////////////////////////////////////////

#ifndef REALM_OBJECT_STORE_HPP
#define REALM_OBJECT_STORE_HPP

#include "property.hpp"

#include <realm/table_ref.hpp>

#include <functional>
#include <vector>

namespace realm {
class Group;
class Schema;
class SchemaChange;
class StringData;
enum class SchemaMode : uint8_t;

namespace util {
template<typename... Args>
std::string format(const char* fmt, Args&&... args);
}

class ObjectStore {
public:
    // Schema version used for uninitialized Realms
    static const uint64_t NotVersioned;

    // get the last set schema version
    static uint64_t get_schema_version(Group const& group);

    // check if all of the changes in the list can be applied automatically, or
    // throw if any of them require a schema version bump and migration function
    static void verify_no_migration_required(std::vector<SchemaChange> const& changes);

    // Similar to above, but returns a bool rather than throwing/not throwing
    static bool needs_migration(std::vector<SchemaChange> const& changes);
    static bool external_changes_are_valid(std::vector<SchemaChange> const& changes);

    // check if any of the schema changes in the list are forbidden in
    // additive-only mode, and if any are throw an exception
    static void verify_valid_additive_changes(std::vector<SchemaChange> const& changes);

    // check if changes is empty, and throw an exception if not
    static void verify_no_changes_required(std::vector<SchemaChange> const& changes);

    // updates a Realm from old_schema to the given target schema, creating and updating tables as needed
    // passed in target schema is updated with the correct column mapping
    // optionally runs migration function if schema is out of date
    // NOTE: must be performed within a write transaction
    static void apply_schema_changes(Group& group, Schema& schema, uint64_t& schema_version,
                                     Schema const& target_schema, uint64_t target_schema_version,
                                     SchemaMode mode, std::vector<SchemaChange> const& changes,
                                     std::function<void()> migration_function={});

    // get a table for an object type
    static realm::TableRef table_for_object_type(Group& group, StringData object_type);
    static realm::ConstTableRef table_for_object_type(Group const& group, StringData object_type);

    // get existing Schema from a group
    static Schema schema_from_group(Group const& group);

    static void set_schema_columns(Group const& group, Schema& schema);

    // deletes the table for the given type
    static void delete_data_for_object(Group& group, StringData object_type);

    // indicates if this group contains any objects
    static bool is_empty(Group const& group);

    // renames the object_type's column of the old_name to the new name
    static void rename_property(Group& group, Schema& schema, StringData object_type, StringData old_name, StringData new_name);

    // get primary key property name for object type
    static StringData get_primary_key_for_object(Group const& group, StringData object_type);

    // sets primary key property for object type
    // must be in write transaction to set
    static void set_primary_key_for_object(Group& group, StringData object_type, StringData primary_key);

    static std::string table_name_for_object_type(StringData class_name);
    static StringData object_type_for_table_name(StringData table_name);

private:
    friend class ObjectSchema;
};

class InvalidSchemaVersionException : public std::logic_error {
public:
    InvalidSchemaVersionException(uint64_t old_version, uint64_t new_version);
    uint64_t old_version() const { return m_old_version; }
    uint64_t new_version() const { return m_new_version; }
private:
    uint64_t m_old_version, m_new_version;
};

class DuplicatePrimaryKeyValueException : public std::logic_error {
public:
    DuplicatePrimaryKeyValueException(std::string object_type, std::string property);

    std::string const& object_type() const { return m_object_type; }
    std::string const& property() const { return m_property; }
private:
    std::string m_object_type;
    std::string m_property;
};

// Schema validation exceptions
struct ObjectSchemaValidationException : public std::logic_error {
    ObjectSchemaValidationException(std::string message) : logic_error(std::move(message)) {}

    template<typename... Args>
    ObjectSchemaValidationException(Args&&... args)
    : std::logic_error(util::format(std::forward<Args>(args)...)) { }
};

struct SchemaValidationException : public std::logic_error {
    SchemaValidationException(std::vector<ObjectSchemaValidationException> const& errors);
};

struct SchemaMismatchException : public std::logic_error {
    SchemaMismatchException(std::vector<ObjectSchemaValidationException> const& errors);
};

struct InvalidSchemaChangeException : public std::logic_error {
    InvalidSchemaChangeException(std::vector<ObjectSchemaValidationException> const& errors);
};
} // namespace realm

#endif /* defined(REALM_OBJECT_STORE_HPP) */
