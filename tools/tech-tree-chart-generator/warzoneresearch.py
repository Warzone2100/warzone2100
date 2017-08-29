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

import re, hashlib
import pydot


PATH='../../data/mp/'
PREREQS='stats/research/multiplayer/prresearch.txt'
NAMES='messages/strings/names.txt'
COSTS='stats/research/multiplayer/research.txt'

class Tech(object):
    def __init__(self, name, rawcosttime):
        self.name = name
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
            elif (self.matches(['cannon', 'mortar', 'shell', 'howitzer', 'pepperpot', 'rail', 'massdriver'])
                    and not self.matches(['plasma', 'emp'])):
                self._color = 'blue'
            elif (self.matches(['mg', 'machinegun']) or
                    (self.matches(['aa']) and not self.matches(['sam', 'laser']))):
                self._color = 'purple'
            elif self.matches(['plasma', 'laser', 'energy']) and not self.matches(['lassat', 'bomb']):
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
            elif self.matches(['bunker', 'hardpoint', 'emplacement', 'aasite', 'sam site', 'fortress', 'battery', 'tower', 'pit']):
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
            STYLES = 'dashed dotted solid'.split()
            hashvalue = int(hashlib.sha256(self.name).hexdigest(), 16)
            style = STYLES[hashvalue % len(STYLES)]
            self._edgestyle = style
        return self._edgestyle

    @property
    def deepprereqs(self):
        x = set(self.prereqs)
        for prereq in self.prereqs:
            x.update(prereq.deepprereqs)
        return x

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

    def __repr__(self):
        return 'Tech(%s, %s)' % (self.name, self.cost)

    def matches(self, names):
        for name in names:
            if name.lower() in self.label.lower() or name.lower() in self.name.lower():
                return True
        else:
            return False

def main():
    techs = {}

    with open(PATH + COSTS) as costs:
        for line in costs:
            x = line.split(',')
            name, cost = x[0], int(x[11])
            if name.startswith('CAM1'):  # Automatic stuff
                continue
            if cost > 450 * 32:
                cost = 450 * 32  # A runtime cap, according to Zarel
            techs[name] = Tech(name, cost)

    from pprint import pprint
    #pprint(techs)

    with open(PATH + PREREQS) as prereqs:
        for line in prereqs:
            name, prereq, junk = line.split(',')
            if name == prereq:  # This tells the game the tech is automatic
                continue
            if prereq.startswith('CAM1'):  # Automatic stuff
                continue
            techs[name].prereqs.add(techs[prereq])
            techs[prereq].dependents.add(techs[name])

    with open(PATH + NAMES) as names:
        for line in names:
            m = re.match(r'^([a-zA-Z0-9-]+)\s+_\("(.*)"\)\s*$', line)
            if m is not None:
                name, label = m.groups()
                if name in techs:
                    techs[name].label = label

    #print techs['R-Vehicle-Body01'].cumcost
    #for tech in techs.itervalues():
    #    print tech, list(tech.prereqs), tech.cumcost

    #sortedtechs = sorted(techs.itervalues(), key=lambda tech: tech.cumcost)
    #for tech in sortedtechs:
    #    print tech.cumcost, tech, tech.prereqs

    graph = pydot.Dot()
    graph.set_ranksep('1.0')
    graph.set_rank('min')
    graph.set_rankdir('LR')
    graph.set_concentrate('true')
    #graph.set_size('60,60')

    automatic = pydot.Cluster()
    automatic.set_label("Automatic")
    automatic.set_fontcolor("darkgrey")  # Lighter than "grey".  Don't ask me why.
    graph.add_subgraph(automatic)

    # Pick out the ones we want
    #chosen_techs = techs.values()
    #import random
    #random.shuffle(techvalues)
    chosen_techs = set(tech for tech in techs.values() if tech.prereqs) # if tech.color == 'red')
    # Recursively grab their dependencies
    dirty = set(chosen_techs)
    while dirty:
        x = set(dirty)
        dirty.clear()
        chosen_techs |= x
        for tech in x:
            dirty.update(tech.prereqs - chosen_techs)

    #print "Retaliation:", techs['R-Vehicle-Body03'].cumcost
    #print "Turbo-Charged Engine:", techs['R-Vehicle-Engine04'].cumcost
    #print "Gas Turbine Engine:", techs['R-Vehicle-Engine07'].cumcost

    chosen_techs = sorted(chosen_techs, key=lambda tech: (tech.cumcost, tech.label))

    # Add nodes
    for tech in chosen_techs:
        node = pydot.Node(tech.name)
        node.set_label(tech.label)
        #node.set_label(tech.name)
        #node.set_label(r'%s\n%s' % (tech.label, tech.name))
        node.set_shape('box')
        node.set_style('filled,setlinewidth(3.0)')
        node.set_color('black')
        node.set_fillcolor('lightgrey')
        node.set_fontsize('24')

        if tech.color is not None:
            node.set_color(tech.color)

        if tech.fillcolor is not None:
            node.set_fillcolor(tech.fillcolor)

        if tech.name in ['R-Sys-Engineering01', 'R-Sys-Sensor-Turret01', 'R-Wpn-MG1Mk1']:
            #node.set_URL(r"//wzguide.co.cc/new/r/sensorturret")
            automatic.add_node(node)
        else:
            graph.add_node(node)

    # Add edges
    for tech in chosen_techs:
        for prereq in tech.prereqs:
            edge = pydot.Edge(prereq.name, tech.name)
            edge.set_tailport('e')
            edge.set_headport('w')

            #if prereq.name in ['R-Struc-Research-Upgrade07', 'R-Sys-Sensor-Upgrade01']:
            #    edge.set_style('dashed')
            #elif prereq.name in ['R-Defense-HardcreteWall', 'R-Struc-Research-Upgrade04']:
            #    edge.set_style('dotted')
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

if __name__ == '__main__':
    main()
