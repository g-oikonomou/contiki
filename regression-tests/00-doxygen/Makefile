# Copyright (c) 2014, Friedrich-Alexander University Erlangen-Nuremberg
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of the University nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.

DOCDIR=../../doc
DOXYGEN_LOG = doxygen.log
DOXYGEN_OUTPUT_FILE = $(DOCDIR)/$(DOXYGEN_LOG)

# We may be running locally, or we may be running on travis. If we are running
# locally, set a correct value for TRAVIS_PULL_REQUEST
ifeq ($(TRAVIS),)
  TRAVIS_PULL_REQUEST=false
endif

# If this is a job triggered by a PR, this will be the log that would get
# generated if we merged the pull and then built docs
# If this is a job triggered by a branch, this will be the log for the branch
# This will always get built, including in local builds
BRANCH_LOG = branch.log

# If this is a job triggered by a PR, this will be the doxygen log that would
# be generated if we built docs for the most recent common ancestor between the
# PR's branch and the PR's base.
# This will not be built for travis jobs triggered by a branch update
# This will not be built for local builds.
BASE_LOG = base.log

ADDED = added
FIXED = fixed

CLEAN_TARGETS = summary $(BRANCH_LOG) $(BASE_LOG) $(ADDED) $(FIXED)

ifeq ($(TRAVIS_PULL_REQUEST),false)
summary: $(BRANCH_LOG)
	echo "Doxygen: OK" > $@
	echo `wc -l < $<` "doxygen warnings" >> $@
	cat $< >> $@
else
summary: $(ADDED) $(FIXED)
	@( \
		if [ -s $(ADDED) ] ; then \
			echo "Doxygen: FAIL ಠ_ಠ" > $@; \
			echo "Warnings added by this pull:" `wc -l < $(ADDED)` >> $@; \
			cat $(ADDED) >> $@; \
		else \
			echo "Doxygen: OK" > $@; \
		fi ; \
		if [ -s $(FIXED) ] ; then \
			echo "Warnings fixed by this pull:" `wc -l < $(FIXED)` >> $@; \
			cat $(FIXED) >> $@; \
		fi ; \
	)

# Some remote references derived from the pull number
PR_HEAD_REFSPEC = refs/pull/$(TRAVIS_PULL_REQUEST)/head
PR_LOCAL_REFSPEC = tmp/travis/pull/$(TRAVIS_PULL_REQUEST)
endif

.PHONY: checkout-common-ancestor clean

# Common rule for all logs
%.log:
	make -C $(DOCDIR) clean
	make -C $(DOCDIR) CONTIKI_DOC_QUIET=YES $(DOXYGEN_LOG)
	cat $(DOXYGEN_OUTPUT_FILE) | grep ": warning: " > $@

# Additional pre-req for BRANCH_LOG, in order to enforce order
$(BASE_LOG): checkout-common-ancestor

$(ADDED): $(FIXED)
	-diff $(BASE_LOG) $(BRANCH_LOG) | grep \> > $@
	
$(FIXED): $(BASE_LOG)
	-diff $(BASE_LOG) $(BRANCH_LOG) | grep \< > $@

# Travis cloned with --depth, therefore it only fetched the branch that is
# pointed to by the remote's HEAD. If the remote's HEAD points to master, and
# the pull is against master, then we are good. However, if the remote's HEAD
# points to some branch and the pull is against a different branch, then we
# need to fetch $(TRAVIS_BRANCH). We also fetch the pull's tip

checkout-common-ancestor: $(BRANCH_LOG)
	git fetch origin +$(PR_HEAD_REFSPEC):$(PR_LOCAL_REFSPEC) +$(TRAVIS_BRANCH):
	$(eval MERGE_BASE := $(shell git merge-base $(PR_LOCAL_REFSPEC) FETCH_HEAD))
	git checkout -q $(MERGE_BASE)

clean:
	rm -f $(CLEAN_TARGETS)
	make -C $(DOCDIR) clean
