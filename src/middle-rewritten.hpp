#ifndef OSM2PGSQL_MIDDLE_REWRITTEN_HPP
#define OSM2PGSQL_MIDDLE_REWRITTEN_HPP

#include <stdio.h>
#include <stdint.h>

#include "middle.hpp"

typedef int64_t s64;

class options_t;

struct relindex_t {
    relindex_t(std::string filename);
    ~relindex_t();

    FILE *f;
    s64 *data;
    int next;
    int size;
};

struct middle_rewritten_t : public middle_t, public middle_query_t {
    middle_rewritten_t(options_t const *options);
    virtual ~middle_rewritten_t() noexcept = default;

    void start() override {}
    void stop(thread_pool_t &pool) override;
    void analyze() override {}
    void commit() override {}
    void flush() override {}

    void node_set(osmium::Node const &node) override;
    void way_set(osmium::Way const &way) override;
    void relation_set(osmium::Relation const &rel) override;

    size_t nodes_get_list(osmium::WayNodeList *nodes) const override;
    bool way_get(osmid_t id, osmium::memory::Buffer &buffer) const override;
    bool relation_get(osmid_t id, osmium::memory::Buffer &buffer) const override;

    idlist_t relations_using_way(osmid_t way_id) const override;

    size_t rel_way_members_get(osmium::Relation const &rel, rolelist_t *roles, osmium::memory::Buffer &buffer) const override;
    std::shared_ptr<middle_query_t> get_query_instance() override;

    s64 next() const;
    void skip(int i) const;

private:
    relindex_t *m_relindex;
};

#endif // OSM2PGSQL_MIDDLE_REWRITTEN_HPP
