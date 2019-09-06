#!/usr/bin/env python
# -*- coding: UTF-8 -*-
#
#    Copyright © 2009 Adam Olsen
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.

from __future__ import with_statement, division

import hashlib
import json
import pydot

PATH = '../../data/mp/'
RESEARCH = 'stats/research.json'


class Tech(object):
    def __init__(self, name, rawcosttime):
        self.name = name
        self.label = None
        self.rawcosttime = rawcosttime
        self.prereqs = set()
        self.dependents = set()
        self._cumcost = None

    @property
    def color(self):
        if not hasattr(self, '_color'):
            if self.matches(['flamer', 'phosphor', 'thermite', 'plasmite']):
                self._color = 'orange'
            elif self.matches(['missile', 'rocket', 'lancer', 'scourge', 'sam']):
                self._color = 'red'
            elif ((self.matches(['cannon', 'rail', 'massdriver']) or
                   self.matches(['mortar', 'shell', 'howitzer', 'pepperpot'])) and not
                  self.matches(['plasma', 'emp'])):
                self._color = 'blue'
            elif (self.matches(['mg', 'machinegun']) or
                  (self.matches(['aa']) and not
                   self.matches(['sam', 'laser']))):
                self._color = 'purple'
            elif (self.matches(['plasma', 'laser', 'energy']) and not
                  self.matches(['lassat', 'bomb'])):
                self._color = 'green'
            else:
                self._color = None
        return self._color

    @property
    def fillcolor(self):
        if not hasattr(self, '_fillcolor'):
            if self.matches(['cyborg']):
                self._fillcolor = 'lightblue'
            elif self.matches(['r-vehicle-body', 'r-vehicle-prop-']):
                self._fillcolor = 'greenyellow'  # Really just light green
            elif self.matches(['r-vehicle-engine']):
                self._fillcolor = '#32cd32'  # limegreen
            elif (self.matches(['hardpoint', 'emplacement', 'tower']) or
                  self.matches(['bunker', 'battery', 'pit']) or
                  self.matches(['aasite', 'sam site', 'fortress'])):
                self._fillcolor = 'peachpuff'
            elif self.matches(['r-struc-research-upgrade']):
                self._fillcolor = 'darkkhaki'
            elif self.matches(['r-struc-power-upgrade']):
                self._fillcolor = 'goldenrod'
            else:
                self._fillcolor = None
        return self._fillcolor

    @property
    def edgestyle(self):
        if not hasattr(self, '_edgestyle'):
            #from random import choice
            styles = 'dashed dotted solid'.split()
            hashvalue = int(hashlib.sha256(self.name.encode('utf-8')).hexdigest(), 16)
            style = styles[hashvalue % len(styles)]
            self._edgestyle = style
        return self._edgestyle

    @property
    def deepprereqs(self):
        pre_reqs_deep = set(self.prereqs)
        for prereq in self.prereqs:
            pre_reqs_deep.update(prereq.deepprereqs)
        return pre_reqs_deep

    @property
    def cost(self):
        # rawcosttime is used for both research time and research cost,
        # but cost divides it by 32, then applies a cap of 450
        return min(self.rawcosttime // 32, 450)

    @property
    def cumcost(self):
        if self._cumcost is None:
            self._cumcost = (self.cost +
                             sum(prereq.cost for prereq in self.deepprereqs))
        return self._cumcost

    def matches(self, names):
        for name in names:
            if name.lower() in self.label.lower() or name.lower() in self.name.lower():
                return True
        return False


def invalid_stat_string(stat_name):
    return stat_name.startswith('CAM')

def setup_tech():
    cost_cap = 450 * 32
    techs = {}

    data = json.load(open(PATH + RESEARCH, 'r', encoding='utf-8'))

    for key, value in data.items():
        if invalid_stat_string(key):
            continue

        cost = 0
        if 'researchPoints' in data[key]:
            cost = data[key]['researchPoints']
            if cost > cost_cap:
                cost = cost_cap # A runtime cap, according to Zarel

        techs[key] = Tech(key, cost)

    # Find research prerequisites
    for key, value in data.items():
        if invalid_stat_string(key):
            continue

        techs[key].label = data[key]['name']

        if 'requiredResearch' in data[key]:
            required_research = data[key]['requiredResearch']

            for res in required_research:
                if invalid_stat_string(res) or (res == key):
                    continue
                techs[key].prereqs.add(techs[res])
                techs[res].dependents.add(techs[key])

    return techs


def find_deps(techs_with_deps):
    tech_copy = techs_with_deps
    dirty = set(tech_copy)
    while dirty:
        deps = set(dirty)
        dirty.clear()
        tech_copy |= deps
        for tech in deps:
            dirty.update(tech.prereqs - tech_copy)
    return tech_copy


def make_the_graph():
    start_tech = ['R-Sys-Engineering01', 'R-Sys-Sensor-Turret01', 'R-Wpn-MG1Mk1']
    techs = setup_tech()

    graph = pydot.Dot()
    graph.set_ranksep('1.0')
    graph.set_rank('min')
    graph.set_rankdir('LR')
    graph.set_concentrate('true')

    automatic = pydot.Cluster()
    automatic.set_label("Automatic")
    automatic.set_fontcolor("darkgrey")  # Lighter than "grey".  Don't ask me why.
    graph.add_subgraph(automatic)

    #get the technologies with dependencies
    chosen_techs = set(tech for tech in techs.values() if tech.prereqs)
    #sort by cumulative cost of the technology
    chosen_techs = sorted(find_deps(chosen_techs), key=lambda tech: (tech.cumcost, tech.label))

    # Add nodes
    for tech in chosen_techs:
        node = pydot.Node(tech.name)
        node.set_label(tech.label)
        node.set_shape('box')
        node.set_style('filled,setlinewidth(3.0)')
        node.set_color('black')
        node.set_fillcolor('lightgrey')
        node.set_fontsize('24')

        if tech.color is not None:
            node.set_color(tech.color)

        if tech.fillcolor is not None:
            node.set_fillcolor(tech.fillcolor)

        if tech.name in start_tech:
            automatic.add_node(node)
        else:
            graph.add_node(node)

    # Add edges
    for tech in chosen_techs:
        for prereq in tech.prereqs:
            edge = pydot.Edge(prereq.name, tech.name)
            edge.set_tailport('e')
            edge.set_headport('w')

            style = prereq.edgestyle
            if prereq.color is None:
                edge.set_color('black')
            else:
                edge.set_color(prereq.color)
            edge.set_style(style)

            #edge.set_weight(str(20.0 / (len(tech.prereqs) + len(prereq.dependents))))
            weight = 8.0 / 1.2**(len(prereq.dependents) + len(tech.prereqs) / 4)
            if tech.color != prereq.color:
                weight /= 2
            if tech.cumcost:
                weight /= (tech.cumcost / 10000)
            edge.set_weight(str(weight))

            graph.add_edge(edge)

    graph.write_svg('warzoneresearch.svg')


def main():
    make_the_graph()


if __name__ == '__main__':
    main()
