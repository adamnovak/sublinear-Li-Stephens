#include "haplotype_manager.hpp"

using namespace std;

size_t haplotypeManager::length() {
  return read_reference.length();
}

size_t haplotypeManager::read_sites() {
  return read_site_read_positions.size();
}

size_t haplotypeManager::shared_sites() {
  reaturn shared_site_read_indices.size();
}

size_t haplotypeManager::ref_position(size_t p) {
  return p + start_position;
}

size_t haplotypeManager::read_position(size_t p) {
  if(p >= start_position && p <= end_position) {
    return p - start_position;
  } else {
    return MAX_SIZE;
  }
}


size_t haplotypeManager::get_read_site_read_position(size_t i) {
  return read_site_read_positions[i];
}

size_t haplotypeManager::get_read_site_ref_position(size_t i) {
  return get_site_position_from_read_position(
          read_site_read_positions[i]);
}

void haplotypeManager::find_shared_sites() {
  for(size_t i = 0; i < read_site_read_positions.size(); i++) {
    read_site_is_shared.push_back(
            reference->is_site(
                  ref_position(read_site_read_positions[i])));
    if(read_site_is_shared[i]) {
      shared_site_read_indices.push_back(i);
    }
  }
}

void haplotypeManager::find_ref_sites_below_read_sites() {
  for(size_t i = 0; i < read_site_read_positions.size(); i++) {
    ref_site_below_read_site.push_back(
            reference->find_site_below(
                    ref_position(read_site_read_positions[i])));
  }
}

void haplotypeManager::build_subsequence_indices() {
  size_t next_read_only = 0;
  size_t next_shared = 0;
  for(size_t i = 0; i < read_site_read_positions.size(); i++) {
    if(read_site_is_shared[i]) {
      subsequence_indices.push_back(next_shared);
      next_shared++;
    } else {
      subsequence_indices.push_back(next_read_only);
      next_read_only++;
    }
  }
}

size_t haplotypeManager::index_among_shared_sites(size_t i) {
  if(read_site_is_shared[i]) {
    return subsequence_indices[i];
  } else {
    return MAX_SIZE;
  }
}

size_t haplotypeManager::index_among_read_only_sites(size_t i) {
  if(!read_site_is_shared[i]) {
    return subsequence_indices[i];
  } else {
    return MAX_SIZE;
  }
}


size_t haplotypeManager::get_shared_site_read_index(size_t j) {
  return shared_site_read_indices[j];
}

size_t haplotypeManager::get_shared_site_ref_index(size_t j) {
  return get_ref_site_below_read_site(get_shared_site_read_index(j));
}

size_t haplotypeManager::get_ref_site_below_read_site(size_t i) {
  return ref_site_below_read_site[i];
}

float haplotypeManager::invariant_penalty_at_read_site(size_t i) {
  if(read_reference == nullptr) {
    return 0;
  } else {
    return (penalties->mu)*invariant_penalties_by_read_site[i];
  }
}

bool haplotypeManager::conatins_shared_sites() {
  return (shared_site_read_indices.size() != 0);
}

bool haplotypeManager::contains_ref_sites() {
  return ref_sites;
}

bool haplotypeManager::contains_read_only_sites() {
  return (shared_site_read_indices.size() != read_site_read_positions.size());
}

void haplotypeManager::check_for_ref_sites() {
  ref_sites = (reference->find_site_above(start_position) != 
          reference->find_site_above(end_position);
}

void haplotypeManager::find_ref_only_sites_and_alleles() {
  if(!contains_ref_sites()) {
    return;
  } else {
    size_t lower_ref_index;
    size_t upper_ref_index;
    
    // initial span
    lower_ref_index = reference->find_site_above(start_position);
    upper_ref_index = get_shared_site_ref_index(0);
    vector<alleleAtSite> to_add;
    for(size_t i = lower_ref_index; i < upper_ref_index; i++) {
      to_add.push_back(alleleAtSite(i, 
              char_to_allele(read_reference->at(read_position(
                      reference->get_position(i))))));
    }
    ref_sites_in_initial_span = to_add;
    
    // spans following read sites i to 1-before-end
    for(size_t i = 0; i < shared_sites() - 1; i++) {
      lower_ref_index = get_shared_site_ref_index(i) + 1;
      upper_ref_index = get_shared_site_ref_index(i + 1);
      to_add.clear();
      for(size_t j = lower_ref_index; j < upper_ref_index; j++) {
        to_add.push_back(alleleAtSite(j, 
                char_to_allele(read_reference->at(read_position(
                        reference->get_position(j))))));
      }
      ref_sites_after_shared_sites.push_back(to_add);
    }
    
    // terminal span
    if(shared_sites() != 0) {
      lower_ref_index = get_shared_site_ref_index(shared_sites() - 1);
      upper_ref_index = reference->find_site_above(end_position);
      to_add.clear();
      for(size_t j = lower_ref_index; j < upper_ref_index; j++) {
        to_add.push_back(alleleAtSite(j, 
                char_to_allele(read_reference->at(read_position(
                        reference->get_position(j))))));
      }
      ref_sites_after_shared_sites.push_back(to_add);
    }
  }
}

void haplotypeManager::count_invariant_penalties() {
  if(read_reference == nullptr) {
    // read reference is the same as the reference structure reference--the only
    // possible deviations are at the read-sites
    return;
  } else {
    size_t running_count = 0;
    size_t count_from;
    size_t count_until;
    
    // initial span
    count_from = start_position;    
    if(read_site_read_positions.size() != 0) {
      count_until = get_read_site_ref_position(0);
    } else {
      count_until = end_position + 1;
    }
    for(size_t p = count_from; p < count_until; p++) {
      if(!reference_sequence-matches(p, 
              char_to_allele(read_reference->at(read_position(p)))) {
        running_count++;
      }
    }
    invariant_penalties_by_read_site.push_back(running_count);
    
    // spans following read sites i to 1-before-end
    for(size_t i = 0; i < read_site_read_positions.size() - 1; i++) {
      size_t count_from = get_read_site_ref_position(i);
      count_until = get_read_site_ref_position(i + 1);
      for(size_t p = count_from; p < count_until; p++) {
        if(!reference_sequence-matches(p, 
                char_to_allele(read_reference->at(read_position(p)))) {
          running_count++;
        }
      }
      invariant_penalties_by_read_site.push_back(running_count);
    }
    
    // terminal span
    if(read_site_read_positions.size() != 0) {
      count_from = get_read_site_ref_position(read_sites() - 1);
      count_until = end_position + 1;
      for(size_t p = count_from; p < count_until; p++) {
        if(!reference_sequence-matches(p, 
                char_to_allele(read_reference->at(read_position(p)))) {
          running_count++;
        }
      }
      invariant_penalties_by_read_site.push_back(running_count);
    }
  }
}

haplotypeManager(
        linearReferenceStructure* reference, haplotypeCohort* cohort, 
              penaltySet* penalties, referenceSequence* reference_sequence,
        vector<size_t> site_positions_within_read,
        string* read_reference = nullptr, size_t start_reference_position = 0) : 
        
        reference(reference), cohort(cohort), penalties(penalties),
              reference_sequence(reference_sequence),
        read_site_positions(site_positions_within_read),
        read_reference(read_reference), 
              start_position(start_reference_position) {
  
  tree = haplotypeStateTree(reference, cohort, penalties);
  end_position = start_position + read_reference->size();
  
  find_ref_sites_below_read_sites();
  find_shared_sites();
  check_for_ref_sites();
  build_subsequence_indices();
  count_invariant_penalties();
  find_ref_only_sites_and_alleles();
}

haplotypeManager::~haplotypeManager() {
  delete tree;
}

void haplotypeManager::initialize_tree() {
  if(!contains_ref_sites()) {
    // there are zero reference sites in all of the read. Clearly no shared 
    // sites either
    
    // initialize tree which consists in its entirety of a single root node
    // whose state is given by a site-less span of length length()
    
    size_t augmentations = invariant_penalties_by_read_site[0];
    size_t read_length = length();
    tree.start_with_span(read_length);
  } else {
    if(reference->get_position(get_shared_site_ref_index(0)) ==
            start_position) {
      return;
    } else {
      if(ref_sites_in_initial_span.size() == 0) {
        // it's just a regular span
        tree.start_with_span(
                reference->get_position(get_shared_site_ref_index(0)) -
                start_position);
      } else {
        // there are some ref sites in here
        if(reference->get_position(ref_sites_in_initial_span[0].site_index) ==
                start_position) {
          tree.start_with_inactive_site(
                  ref_sites_in_initial_span[0].site_index,
                  ref_sites_in_initial_span[0].allele);
        } else {
          size_t left_span_length =
                  reference->get_position(
                          ref_sites_in_initial_span[0].site_index) -
                  start_position;
          tree.start_with_span(left_span_length);
          tree.extend_node_by_allele_at_site(root,
                  ref_sites_in_initial_span[0].site_index,
                  ref_sites_in_initial_span[0].allele);
        }
        for(size_t i = 1; i < ref_sites_in_initial_span.size(); i++) {
          tree.extend_node_by_allele_at_site(root,
                  ref_sites_in_initial_span[i].site_index,
                  ref_sites_in_initial_span[i].allele);
        }
      }
    }
  }
}