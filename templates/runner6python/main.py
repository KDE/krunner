#!/usr/bin/python3

from typing import List
from krunnerdbusutils import krunner_actions, krunner_match, krunner_run, \
    AbstractRunner, Action, Match, run_event_loop


class Runner(AbstractRunner):
    def __init__(self):
        super().__init__("org.kde.%{APPNAMELC}")

    @krunner_match
    def Match(self, query: str) -> List[Match]:
        """This method is used to get the matches and it returns a list of tupels"""
        matches: List[Match] = []
        if query == "hello":
            match = Match()  # Or utilize keyword constructor
            match.id = "hello_match"
            match.text = "Hello There!"
            match.subtext = "Example"
            match.icon = "planetkde"
            matches.append(match)
        return matches

    @krunner_actions
    def Actions(self) -> List[Action]:
        return [Action(id="id", text="Action Tooltip", icon="planetkde")]

    @krunner_run
    def Run(self, data: str, action_id: str):
        print(data, action_id)


if __name__ == "__main__":
    run_event_loop(Runner)
